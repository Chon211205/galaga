//Librerias utilizadas
#include <iostream>
#include <fstream> //libreria para manejo de archivos.
#include <sstream> //libreria para menojo de flujos de memoria
#include <vector> //libreria para trabajar con contenedores dinamicos stl
#include <algorithm> //libreria para trabajar con algortimos genericos para contenedores stl.
#include <ctime> //libreria para manejo del tiempo y la fecha.
#include <string>
#include <iomanip>
#include <pthread.h> //libreria para trabajar con los hilos POSIX
#include <unistd.h> //liberia de funciones estandar de sistema POSIX
#include <termios.h> //proporciona funciones y estructuras para controlar la configuracion de la terminal.
#include <fcntl.h> //controla archivos y descriptores.

using namespace std;

//Definicion de colores para el entorno
#ifndef ANSI_RESET
#define ANSI_RESET   "\033[0m"
#endif
#ifndef ANSI_BOLD
#define ANSI_BOLD    "\033[1m"
#endif
#ifndef ANSI_WHITE
#define ANSI_WHITE   "\033[37m"
#endif
#ifndef ANSI_GREEN
#define ANSI_GREEN   "\033[32m"
#endif
#ifndef ANSI_YELLOW
#define ANSI_YELLOW  "\033[33m"
#endif
#ifndef ANSI_MAGENTA
#define ANSI_MAGENTA "\033[35m"
#endif
#ifndef ANSI_RED
#define ANSI_RED     "\033[31m"
#endif

//Interfaz de usuario principal.
static inline void ui_cls() { printf("\x1b[2J\x1b[H"); } //limpia toda la pantalla
static inline void ui_hideCursor(){ printf("\x1b[?25l"); } //Oculta el cursor de la terminal
static inline void ui_showCursor(){ printf("\x1b[?25h"); } //Vuelve a mostrar el cursor en la terminal
static inline void ui_invertOn(){ printf("\x1b[7m"); } //Intercambio los colores en la terminal, el fondo y texto se intercambian
static inline void ui_invertOff(){ printf("\x1b[27m"); (void)ui_invertOff; } //Desactiva el intercambio de colores en la terminal.
static const int UI_COLS = 80; //Numero fijo de columnas de la terminal usdas como referencia para centrar los textos.

//Calculo de la columna inicial en base al ancho de la terminal.
static inline void ui_center(int row, const std::string& s, bool bold=false){
    int col = std::max(1, (UI_COLS - (int)s.size())/2);
    printf("\x1b[%d;%dH", row, col);
    if(bold) std::cout << ANSI_BOLD << s << ANSI_RESET;
    else     std::cout << s;
}

//Dibujo de los bordes de la interfaz de usuario inicial ("-" para las lineas horizontales, "|" para las verticales y "+" para las esquinas)
static inline void ui_box(int top, int left, int w, int h, const char* color = ANSI_WHITE){
    printf("\x1b[%d;%dH", top, left);
    std::cout << color << "+" << std::string(w-2,'-') << "+";
    for(int r=1;r<h-1;r++){
        printf("\x1b[%d;%dH", top+r, left);
        std::cout << "|" << std::string(w-2,' ') << "|";
    }
    printf("\x1b[%d;%dH", top+h-1, left);
    std::cout << "+" << std::string(w-2,'-') << "+" << ANSI_RESET;
}

//Dibujo del titulo del juego.
static inline void ui_title(int row=2){
    ui_center(row+0,  std::string(ANSI_YELLOW)+R"(  ____       _                       )"+ANSI_RESET);
    ui_center(row+1,  std::string(ANSI_YELLOW)+R"( / ___| __ _| | __ _  __ _  __ _ _)"+ANSI_RESET);
    ui_center(row+2,  std::string(ANSI_MAGENTA)+R"(| |  _ / _` | |/ _` |/ _` |/ _` |)"+ANSI_RESET);
    ui_center(row+3,  std::string(ANSI_MAGENTA)+R"(| |_| | (_| | | (_| | (_| | (_| |)"+ANSI_RESET);
    ui_center(row+4,  std::string(ANSI_YELLOW)+R"( \____|\__,_|_|\__,_|\__, |\__,_| )"+ANSI_RESET);
    ui_center(row+5,  std::string(ANSI_YELLOW)+R"(                      |___/            )"+ANSI_RESET);
}

// Lectura de tecla bloqueante para el menu.
static int ui_readKeyBlocking(){
    termios oldt; tcgetattr(STDIN_FILENO, &oldt);

    termios raw = oldt;
    raw.c_lflag &= ~(ICANON | ECHO); //Desactivacion del modo canonico y echo.
    raw.c_cc[VMIN]  = 1;   // espera AL MENOS 1 byte
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    unsigned char c=0, s1=0, s2=0;
    int k=0;

    if(read(STDIN_FILENO,&c,1)==1){
        if(c==0x1b){
            raw.c_cc[VMIN]=0; raw.c_cc[VTIME]=1; // tiempo de espera de 1.
            tcsetattr(STDIN_FILENO, TCSANOW, &raw);
            read(STDIN_FILENO,&s1,1);
            read(STDIN_FILENO,&s2,1);
            if(s1=='[' && s2=='A') k='U';   
            else if(s1=='[' && s2=='B') k='D'; 
            else k=27; // ESC
        } else {
            k=c;
        }
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt); //Restauracion a configuraciones originales.
    return k;
}


//---entorno de puntajes---

//Creacion de estructura para los puntajes, recibiendo como parametros:
//nombre del jugador, puntaje, ronda, tiempo, fecha del puntaje.
struct registroPuntaje { string nombre; int puntaje; int ronda; int tiempo; time_t fecha; };

//Crea un directorio donde guarda los puntajes de los jugadores
string rutaPuntaje() {
    const char* ruta = getenv("HOME"); //busca la variable de entorno HOME para construir la ruta
    return string (ruta ? ruta : ".") + "/.puntajes_galaga"; //ruta de los puntajes creada
}

//extraccion de datos para introducirlos al vector, ordenar puntajes.
vector<registroPuntaje> cargarPuntaje() {
    vector<registroPuntaje> v;  //Creacion del vector para almacenar los jugadores
    ifstream in(rutaPuntaje()); //apertura del archivo de la ruta de rutaPuntaje
    if(!in) return v; //Comprueba si existe vector, retorna uno vacio.
    string line; //Guarda cada linea del archivo leida.

    //Lectura de los datos del archivo
    while(getline(in, line)) {
        if(line.empty()) continue; //salta a la siguiente iteracion al estar vacio.
        stringstream ss(line); //Convierte la linea de flujo de texto para extraerlos en campos diferentes
        registroPuntaje r{}; string part;

        //extrae los datos como nombre, puntaje, ronda, tiempo y fecha.
        if (!getline(ss, r.nombre, '|')) continue; 
        if (!getline(ss, part, '|')) continue; r.puntaje = stoi(part); //stoi - conversion de string a integer
        if (!getline(ss, part, '|')) continue; r.ronda = stoi(part); //stoi - conversion de string a integer
        if (!getline(ss, part, '|')) continue; r.tiempo = stoi(part); //stoi - conversion de string a integer
        if (!getline(ss, part, '|')) continue; r.fecha = (time_t)stoll(part); //stoll - conversion de stoll a long long

        v.push_back(r); //Lo manda al registro completo en el vector puntajesJugadores.
    }

    //ordena los jugadores en base al puntaje y ronda del jugador.
    sort(v.begin(), v.end(), [](const registroPuntaje&a, const registroPuntaje&b) {
        if (a.puntaje != b.puntaje) return a.puntaje > b.puntaje;
        if (a.ronda != b.ronda)     return a.ronda   > b.ronda;
        return a.tiempo < b.tiempo;
    });

    //Redimensionar la lista de jugadores en la tabla de puntaje si pasa mayor de 50.
    if (v.size() > 50) v.resize(50);
    return v;
}

//---Guardar puntaje---

//se abre en modo de adicion (ios :: app), esto evita sobreescribir los registros previos.
void guardarPuntaje(const string& nombre, int puntaje, int ronda, int tiempo) {
    //Al no ser posible abrir el archivo, cierra sin afectar el flujo del programa.
    ofstream out (rutaPuntaje(), ios::app);
    if(!out) return;
    time_t now = time(nullptr);

    //Copia el nombre para evitar caracteres conflictivos.
    string safe = nombre;
    for(char& c : safe) if(c == '|' || c == '\n' || c == '\r') c = '-';

    //si el usuario no ingresa su nombre, este es el nombre por defecto.
    if(safe.empty()) safe = "Jugador";

    //Output de los registros en el archivo.
    out << safe << '|' << puntaje << '|' << ronda << '|' << tiempo << '|' << (long long) now << "\n";
}

//---declaracion de funciones---.
void Game(int modoSeleccion);
void instrucciones();
void mostrarPuntajes();
void juego();
int Menu();

//---funcion de instrucciones---.
void instrucciones(){
    bool drawn=false;
    while(true){
        if(!drawn){
            ui_cls(); ui_hideCursor();
            ui_title(2);
            ui_box(10, 10, 60, 12, ANSI_WHITE);

            ui_center(11, std::string(ANSI_BOLD) + "      INSTRUCCIONES" + ANSI_RESET);
            ui_center(13, std::string("                  Mover: ") + ANSI_YELLOW + "A/D" + ANSI_RESET +
                            "   Disparar: " + ANSI_YELLOW + "Espacio" + ANSI_RESET +
                            "   Salir: " + ANSI_YELLOW + "Q" + ANSI_RESET);
            ui_center(14, std::string("                 Modos: ") + ANSI_MAGENTA + "1) 40 " + ANSI_RESET +
                            " " + ANSI_MAGENTA + "2) 50 " + ANSI_RESET +
                            " " + ANSI_MAGENTA + "4) 60 " + ANSI_RESET +
                            " " + ANSI_MAGENTA + "I) Infinito" + ANSI_RESET);
            ui_center(16, std::string("          Cada alien destruido suma 10 puntos. Inicias con ") +
                            ANSI_GREEN + "3 vidas" + ANSI_RESET + ".");
            ui_center(20, std::string(ANSI_WHITE) + "   [ENTER] Volver al menú" + ANSI_RESET);
            std::cout.flush();
            drawn=true;
        }
        int k = ui_readKeyBlocking();
        if(k=='\n'||k=='\r' || k==27){ ui_showCursor(); break; }
    }
}

//---Funcion de mostrar puntajes---.
void mostrarPuntajes(){
    bool drawn=false; //Bandera para dibujar la pantalla
    while(true){
        if(!drawn){
            ui_cls(); ui_hideCursor(); //Limpia la pantalla y oculta el cursor.
            ui_title(2); //Muestra el titulo en la fila 2
            ui_box(9, 8, 64, 14, ANSI_WHITE); //Dibuja la caja donde se muestran los puntajes.
            ui_center(10, std::string(ANSI_BOLD) + "MEJORES PUNTAJES (Top 10)" + ANSI_RESET); //Texto central
            
            //carga los puntajes desde un archivo o memoria.
            auto v = cargarPuntaje();
            if(v.empty()){
                ui_center(14, "No existen datos de jugadores actualmente.");

            }else{
                int n = std::min<int>(10, v.size()), row = 12;
                for(int i=0;i<n;i++){
                    auto &r = v[i];
                    //Muestra la fecha en la que el jugador registro su record.
                    tm* t = localtime(&r.fecha); char buf[32]; strftime(buf,sizeof(buf),"%Y-%m-%d", t);

                    //Output de los datos.
                    std::ostringstream line;
                    line << std::setw(2) << (i+1) << ") "
                         << std::left << std::setw(10) << r.nombre
                         << "  Pts:" << std::setw(5) << r.puntaje
                         << "  Oleada:" << std::setw(3) << r.ronda
                         << "  Tiempo:" << std::setw(4) << r.tiempo << "s  "
                         << buf;
                    ui_center(row+i, line.str());
                }
            }
            //Intruccion para volver al menu principal
            ui_center(22, std::string(ANSI_WHITE) + "[ENTER] Volver al menú" + ANSI_RESET);
            std::cout.flush();
            drawn=true; //Marca la pantalla dibujada.
        }

        //Tecla del usuario bloqueante.
        int k = ui_readKeyBlocking();
        //Salir si presiona ENTER, RETURN o ESC.
        if(k=='\n'||k=='\r' || k==27){ ui_showCursor(); break; }
    }
}

//---Funcion del juego---
void juego(){

    //Output de los modos de juego y en la linea donde se mostrara.
    const std::vector<std::pair<std::string,int>> modos = {
        {"Modo 1: 40 alienígenas (5x8)", 1},
        {"Modo 2: 50 alienígenas (5x10)", 2},
        {"Modo 4: 60 alienígenas (5x10)", 3},
        {"Modo Infinito: oleadas ilimitadas", 0}
    };
    int sel = 0; bool dirty = true;


    while(true){
        if(dirty){
            ui_cls(); ui_hideCursor();
            ui_title(2);
            ui_box(10, 12, 56, 12, ANSI_WHITE);
            ui_center(11, std::string(ANSI_BOLD) + "       SELECCIONA UN MODO" + ANSI_RESET);
            
            //Output del disenio de menu de seleccion de modo de juego.
            for(size_t i=0;i<modos.size();++i){
                std::string line = "  " + modos[i].first + "  ";
                int row = 13 + (int)i;
                int col = std::max(1, (UI_COLS - (int)line.size())/2);
                printf("\x1b[%d;%dH", row, col);
                if((int)i==sel){ ui_invertOn(); std::cout << line << ANSI_RESET; }
                else            std::cout << line;
            }
            ui_center(20, std::string(ANSI_WHITE) + "           ↑/↓ o W/S • ENTER para iniciar" + ANSI_RESET);
            std::cout.flush();
            dirty = false;
        }

        //Teclas bloqueantes para la interaccion del usuario con el juego.
        int k = ui_readKeyBlocking();
        if(k=='U'||k=='w'||k=='W'){ sel = (sel+3)%4; dirty = true; }
        else if(k=='D'||k=='s'||k=='S'){ sel = (sel+1)%4; dirty = true; }
        else if(k=='\n'||k=='\r'){ ui_showCursor(); Game(modos[sel].second); return; }
        else if(k==27){ ui_showCursor(); return; } // ESC para volver
    }
}

//---Funcion de menu---
int Menu(){
    const std::vector<std::string> items = {"Jugar","Instrucciones","Mejores puntajes","Salir"};
    int sel = 0; //Indice de la opcion seleccionada
    bool dirty = true; //Bandera del dibujo de pantalla


    while(true){
        if(dirty){
            ui_cls(); ui_hideCursor(); //Limpia la pantalla y oculta el cursor.
            ui_title(2); //Muestra el titulo.
            ui_box(10, 18, 44, 10, ANSI_WHITE); //Dibuja el entorno del menu.
            ui_center(11, std::string(ANSI_GREEN) + "            == MENU PRINCIPAL ==" + ANSI_RESET, true); //Titutlo del entorno.
            
            //Muestra las opciones del menu.
            for(size_t i=0;i<items.size();++i){
                std::string line = "  " + items[i] + "  ";
                int row = 13 + (int)i;
                int col = std::max(1, (UI_COLS - (int)line.size())/2);
                printf("\x1b[%d;%dH", row, col);

                //Inversion de color a la opcin seleccionada.
                if((int)i==sel){ ui_invertOn(); std::cout << line << ANSI_RESET; }
                else            std::cout << line;
            }

            //Instrucciones de control.
            ui_center(18, std::string(ANSI_WHITE) + "                Usa ↑/↓ o W/S • ENTER para seleccionar" + ANSI_RESET);
            std::cout.flush();
            dirty = false; //bandera del dibujo de pantalla.
        }

        //Teclas bloqueantes para la interaccion del usuario con el juego.
        int k = ui_readKeyBlocking(); 
        if(k=='U'||k=='w'||k=='W'){ sel = (sel + (int)items.size() - 1) % (int)items.size(); dirty = true; }
        else if(k=='D'||k=='s'||k=='S'){ sel = (sel + 1) % (int)items.size(); dirty = true; }
        else if(k=='\n'||k=='\r'){
            ui_showCursor();
            if(sel==0){ ui_cls(); juego(); dirty = true; }
            else if(sel==1){ ui_cls(); instrucciones(); dirty = true; }
            else if(sel==2){ ui_cls(); mostrarPuntajes(); dirty = true; }
            else if(sel==3){ ui_cls(); return 0; }
        }
    }
}

//---Hilos---

//Volatile bool indica al compilador que no optimice el acceso a esa variable, porque puede cambiar externamente.
volatile bool hilo = true;

//Funcion que utilizara para pthread_create
void* heartbeat(void*) {
    while (hilo) { usleep(200000); } //pausa la ejecucion del hilo por 0.2 segundos
    return nullptr;
}

//---Main---
int main(){
    srand(time(0));
    pthread_t th;
    pthread_create(&th,nullptr,heartbeat,nullptr);
    Menu();
    hilo = false;
    pthread_join(th,nullptr);
    ui_cls();
    cout<<"¡Gracias por jugar!\n";
    return 0;
}

//---Juego completo en una funcion---.
void Game(int modoSeleccion) {
    const int width = 40, height = 20;

    /**
     * Estructura que encapsula todo el estado del juego,
     * esta funcion permite centralizar todas las variables que son representadas en un estado actual de una partida como
     * los datos de jugador, enemigos, balas, modo de juego, tiempo y la sincronizacion.
     */ 

    struct GameState {
       //Banderas de estado del juego
        bool gameover = false, gamewin = false, modoInfinito = false; //Modo fin de juego, juego ganado, y modo infinito o arcade

        //Datos del jugador
        int posicionX = width/2, posicionY = height -1; //Posiciones iniciales en X y Y del jugador.
        int vidas = 3, punteo = 0, enemigos = 0, ronda = 1; //datos del jugador: vida, puntaje, enemigos destruidos, ronda actual.

        //Datos de los enemigos  
        vector<int> enemigoX, enemigoY; vector<bool> enemigoEstado; //Posicion en X y Y del enemigo, y actividad de cada enemigo.

        //Balas  
        vector<int> balaX, balaY, balaEnemigoX, balaEnemigoY; //Posicion de las balas del jugador y enemigo en X y Y.

        //modo finito
        int objetivoEnemigo = 0, cantidadGrupo = 0, grupo = 0; //Cantidad de enemigos a destruir, tamanio de cada grupo, y numero de grupos de enemigos generados.
        
        //Aparicion y movimiento del enemigo
        int aparicionEnemigo = 0, siguienteAparicionRetardo = 1; //cantidad de grupos generados, retraso de aparicion de los enemigos
        const int retraseDeGrupo = 60; //cuadro de espera con 3 segundos entre los grupos de enemigos.
        int velocidadEnemigo = 15, direccionEnemigo = 1;

        //Disparo del jugador
        bool disparar = false; const int limiteBalas = 3; //indicacion de disparo del jugador, y limite de balas que dispara el jugador.

        //Tiempo y sincronizacion
        time_t inicio = 0; pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER; //Tiempo en que inicia la partida, y se utiliza mutex para la sincronizacion en el entorno en el que encuentra.
        
        //Terminal Raw
        ::termios orig{}; bool have_orig = false, raw_on = false; //Estado original de la terminal, indicacion de guardado de la configuracion original de la terminal, e indicador si la terminal esta en modo raw actualmente.
    } st;

    /**Configuracion de modos (40, 50, 60 enemigos o modo infinito)
    * Se utilizan los datos general asignados anteriormente para configurar todos los modos de juego, teniendo un if-else, siendo cada uno un modo de juego.
    */

    //Configuracion modo infinito
    if(modoSeleccion == 0) {
        //Confirma que si es el modo infinito
        st.modoInfinito = true; st.objetivoEnemigo = 0; st.cantidadGrupo = 0; st.grupo = -1; 

     //Configuracion modo 40 enemigos. 
    } else if(modoSeleccion == 1) {
        st.modoInfinito = false; st.objetivoEnemigo = 40; st.cantidadGrupo = 8; st.grupo = 5;

    //Configuracion modo 50 enemigos
    } else if (modoSeleccion == 2) {
        st.modoInfinito = false; st.objetivoEnemigo = 50; st.cantidadGrupo = 10; st.grupo = 5;

     //Configuracion modo 60 enemigos
    } else if (modoSeleccion == 3) {
        st.modoInfinito = false; st.objetivoEnemigo = 60; st.cantidadGrupo = 10; st.grupo = 5;
    } else {
        st.modoInfinito = true; //fallback modo infinito.
    }

    /**Se utiliza las secuencias ANSI para limpiar contenido, ocultar el curosr, y mostrar el cursor*/ 

    //Limpia la pantalla en la terminal y coloca el cursor en una esquina, utiliza la secuencia ANSI "\x1b[2J\x1b[H" para limpiar el contenido.
    auto clearScreen = [](){ cout << "\x1b[2J\x1b[H"; };
   //Oculta el cursor en la terminal, utilizando la secuencia ANSI "\x1b[?25l" para mejorar la estetica del juego y animacion.
    auto hideCursor  = [](){ cout << "\x1b[?25l"; };
    //muestra otra vez el cursor en la terminal al finalizar la ejecucion del programa, se utiliza el codigo "\x1b[?25h".
    auto showCursor  = [](){ cout << "\x1b[?25h"; };

    /**Configuracon de las teclas para el juego. */

    //restaura la configuracion original en modo normal y lectura bloqueante.
    auto disableRaw = [&](){
        if(!st.raw_on) return; //verifica si el modo raw esta activo.
        if(st.have_orig) tcsetattr(STDIN_FILENO, TCSAFLUSH, &st.orig); //evalua si tiene guardada la configuracion original con st.have_orig, y lo restaura con 'tcsetattr' de la libreria termios
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0); //Recupera los flags actuales de entrada con 'fcntl' de la libreria termios
        if(flags >= 0) fcntl(STDIN_FILENO, F_SETFL, flags & ~O_NONBLOCK); //Al ser validado, deshabilita el flag 'O_NONBLOCK' para que la lectura de teclado vuelvan a ser bloqueantes.
        st.raw_on = false; //marca false el st.raw_on.
    };

   //Pone la terminal en modo raw que tiene una lectura inmediata y sin bloqueante.
    auto enableRaw = [&](){
        if(!st.have_orig){ tcgetattr(STDIN_FILENO, &st.orig); st.have_orig=true; }  //al no tener guardada la configuracion original de la terminal, la obtiene con 'tcgetattr' y la guarda en 'st.orig'.
        ::termios raw = st.orig; //crea una copia de la configuracion y modifica las flags.
        raw.c_lflag &= ~(ICANON | ECHO); //desactivando ICANON (lectura caracter por caracter) y ECHO (evita la impresion de las teclas en la pantalla.)
        raw.c_cc[VMIN]=0; raw.c_cc[VTIME]=0; //VMIN (no requiere minimo de caracteres) y VTIME (no hay retardo en la lectura)
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw); //Aplica las configuraciones ajustadas anteriormente co 'tcsetattr'
        int flags = fcntl(STDIN_FILENO, F_GETFL, 0); //Con fcntl modifica los flags del descriptor de entrada.
        fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK); //Habilita el modo no bloqueante 'O_NONBLOCK'
        st.raw_on=true; //marca true el st.raw_on.
    };

    /**Genera un grupo de enemigos y los agrega al estado del juego construido anteriormente */
    auto aparicionGrupo = [&](int n) {
        if (n < 0) return; //si se cumple esta condicon, no acciona y retorna de manera instantanea.
        int usable = width -10; //calcula el ancho de lo que se utilizara dejando margenes.
        st.enemigoX.reserve(st.enemigoX.size() + n); //Reserva memoria adicional en los vectores de 'enemigoX'
        st.enemigoY.reserve(st.enemigoY.size() + n); //Reserva memoria adicional en los vectores de 'enemigoY'
        st.enemigoEstado.reserve(st.enemigoEstado.size() + n); //Reserva memoria adicional en 'enemigoEstado' para evitar realocaciones.

        //Se utiliza un ciclo for para calcular crear y calcular el espacio de aparicion del enemigo, teniendo una distribucion uniforme a lo ancho del tablero de juego.
        for (int j = 0; j < n; j++){
            int x = 5 + j*(usable/max(n,1)); //Calculo en posicion X
            st.enemigoX.push_back(x);
            st.enemigoY.push_back(2); //Fijacion de posicion en Y
            st.enemigoEstado.push_back(true);  //marca comoa ctivo el enemigo con true.
        }
    };

    /**Configuracion de disparo de los enemigos */
    auto disparoEnemigo = [&](){
        //Itera sobre los enemigos en X y Y.
        for (size_t i = 0; i < st.enemigoX.size(); i++) {
            //Evalua si el enemigo esta activo para evaluar la probabilidad de disparo, con 2% de probabilidad.
            if (st.enemigoEstado[i] && rand()%100 < 2) {
                //Si se cumple la condicion, se agrega la posicion X del disparo al igual que en Y. 
                //Los guarda en balaEnemigoX y balaEnemigoY, banderas creadas anteriormente.
                st.balaEnemigoX.push_back(st.enemigoX[i]);
                st.balaEnemigoY.push_back(st.enemigoY[i]+1); //Lo agrega justo debajo den enemigo.
            }
        }
    };

    /**Calcula el tamanio del grupo que se va generar en modo infinito, ya que no tiene asignado una determinada como en los otros modos*/
    auto calculoGrupoInfinito = [&](int ronda){
        //Se asigna como minimo 8 enemigos, posteriormente va aumentando hasta llegar a un maximo de 16 enemigos.
        int base = 8, inc = ronda/2, cap = 16; //aumento proporcional a la ronda y se asigna un maximo de 16 enemigos.
        int n = base + inc; if(n > cap) n = cap; return n;
    };

    /**Reinicio de juego al terminar una partida o cuando el jugador pierde*/
    auto reiniciar = [&](){
        st.gameover = false; st. gamewin = false; //Reinicio de estados de juego.
        st.posicionX = width/2; st.posicionY = height-1; //Reinicio de posicion de jugador
        st.punteo = 0; st.vidas = 3; st.enemigos = 0; st.ronda = 1; //Reinicio de datos de jugador
        st.velocidadEnemigo = 15; st.direccionEnemigo = 1; //Reinicio de datos de enemigos
        st.enemigoX.clear(); st.enemigoY.clear(); st.enemigoEstado.clear(); //Reinicio de posicion y estado del enemigo.
        st.balaX.clear(); st.balaY.clear(); //Reinicio de las balas de  jugador
        st.balaEnemigoX.clear(); st.balaEnemigoY.clear(); //Reinicio de las balas del enemigo
        st.aparicionEnemigo = 0; st.siguienteAparicionRetardo = 1; //Reinicio de la aparicion de enemigos.
        st.inicio = time(nullptr); //Reinicio de tiempo y sincronizacion.
    };

    /**Funcion de dibujo para el juego, asignando los colores con ANSI asigandos al inicio del codigo
     * #ifndef ANSI_RESET
        #define ANSI_RESET   "\033[0m"
        #endif
        #ifndef ANSI_BOLD
        #define ANSI_BOLD    "\033[1m"
        #endif
        #ifndef ANSI_WHITE
        #define ANSI_WHITE   "\033[37m"
        #endif
        #ifndef ANSI_GREEN
        #define ANSI_GREEN   "\033[32m"
        #endif
        #ifndef ANSI_YELLOW
        #define ANSI_YELLOW  "\033[33m"
        #endif
        #ifndef ANSI_MAGENTA
        #define ANSI_MAGENTA "\033[35m"
        #endif
        #ifndef ANSI_RED
        #define ANSI_RED     "\033[31m"
        #endif
     */

    auto dibujo = [&](){
        clearScreen(); //llama a la funcion de clearScreen, que trabaja con ANSI.

        //Asignacion de colores al marco superior
        for(int i=0;i<width+2;i++) cout<<ANSI_WHITE<<"#"<<ANSI_RESET;
        cout<<"\n";

        //Asignacion de colores al area de juego.
        for (int i = 0; i < height; i++){
            for (int j = 0; j < width; j++){
                if (j == 0) cout << ANSI_WHITE << "#" << ANSI_RESET; //Pinta los bordes de blanco que estan representados con "#"

                if (i == st.posicionY && j == st.posicionX){
                    cout << ANSI_GREEN << ANSI_BOLD << "A" << ANSI_RESET; //Pinta de verde y asigna negrilla para el jugador representado con "A"
                }
                else {
                    bool drawn = false;

                    for (size_t k = 0; k < st.balaY.size(); k++){
                        if(st.balaY[k] == i && st.balaX[k] == j) {
                            cout << ANSI_YELLOW << "|" << ANSI_RESET; //Pinta las balas del jugador de amarillo representado con "|"
                            drawn = true; break;
                        }
                    }

                    if (drawn) { if (j == width-1) cout << ANSI_WHITE << "#" << ANSI_RESET; continue; }

                    for (size_t k = 0; k < st.balaEnemigoY.size(); k++) {
                        if(st.balaEnemigoY[k] == i && st.balaEnemigoX[k] == j){
                            cout << ANSI_RED << "O" << ANSI_RESET; //Pinta las balas enemigas de color rojo representadas con "o"
                            drawn = true; break;
                        }
                    }

                    if (drawn) { if (j == width-1) cout << ANSI_WHITE << "#" << ANSI_RESET; continue; }

                    for (size_t k = 0; k < st.enemigoY.size(); k++){
                        if(st.enemigoEstado[k] && st.enemigoY[k] == i && st.enemigoX[k] == j) {
                            cout << ANSI_MAGENTA << "V" << ANSI_RESET; //Pinta los enemigos de magenta representados con "V"
                            drawn = true; break;
                        }
                    }

                    if(!drawn) cout << " ";
                    if(j == width-1) cout << ANSI_WHITE << "#" << ANSI_RESET;
                }
            }
            cout << "\n";
        }

        //Asignacion de colores al marco inferior
        for(int i=0;i<width+2;i++) cout<<ANSI_WHITE<<"#"<<ANSI_RESET;
        cout<<"\n";

        //Asignacion de colores al HUD
        int secs = (int)(time(nullptr)-st.inicio);
        if(st.modoInfinito){
            //HUD de modo infinito
            cout << ANSI_BOLD << "Modo:" << ANSI_RESET << " infinito "
                << ANSI_BOLD << "Punteo:" << ANSI_RESET << " " << ANSI_YELLOW << st.punteo << ANSI_RESET << " "
                << ANSI_BOLD << "Vidas:" << ANSI_RESET << " " << ANSI_GREEN << st.vidas << ANSI_RESET << " "
                << ANSI_BOLD << "Ronda:" << ANSI_RESET << " " << ANSI_MAGENTA << st.ronda << ANSI_RESET << " "
                << ANSI_BOLD << "Tiempo:" << ANSI_RESET << " " << secs << "s \n";
        } else {
            //HUD de modo finito (40, 50, 60 enemigos)
            cout << ANSI_BOLD << "Modo:" << ANSI_RESET << " finito "
                << ANSI_BOLD << "Punteo:" << ANSI_RESET << " " << ANSI_YELLOW << st.punteo << ANSI_RESET << " "
                << ANSI_BOLD << "Vidas:" << ANSI_RESET << " " << ANSI_GREEN << st.vidas << ANSI_RESET << " "
                << ANSI_BOLD << "Enemigos destruidos:" << ANSI_RESET << " " << ANSI_MAGENTA << st.enemigos << ANSI_RESET
                << "/" << st.objetivoEnemigo << " "
                << ANSI_BOLD << "Tiempo:" << ANSI_RESET << " " << secs << "s \n";
        }
        cout.flush(); // FIX: era 'count.flush()'
    };

    /**Logica de interaccion del juego */

    auto logica = [&](){
        /**Funcion de control de aparicion de enemigos en base al modo de juego seleccionado */
        
        //Condicional para evaluar si es modo infinito o finito
        if (st.modoInfinito){
            bool modo = false; for (bool a:st.enemigoEstado) if (a){ modo = true; break;} //verifica si hay enemigos activos en la pantalla

            //Configuracion modo infinito
            if(!modo) {
                //si no hay enemigos, y siguienteAparicionRetardo es igual a 0, reinicia el retraseDeGrupo.
                if (st.siguienteAparicionRetardo == 0) st.siguienteAparicionRetardo = st.retraseDeGrupo;

                //si siguienteAparicionRetardo es mayor que 0, reduce los enemigos
                else if (st.siguienteAparicionRetardo > 0) {
                    st.siguienteAparicionRetardo--;

                    //Al llegar a 0 siguienteAparicionRetardo, calcula el tamanio del nuevo grupo con la funcion creada anteriormente calculoGrupoInfinito.
                    if(st.siguienteAparicionRetardo == 0){
                        int n = calculoGrupoInfinito(st.ronda);
                        aparicionGrupo(n); //LLama a la funcion de aparicion de grupo para generar los enemigos.
                        if(st.velocidadEnemigo > 6 && st.ronda%2 == 0) st.velocidadEnemigo--; //Ajusta la velocidad de los enemigos, tiene un limite de velocidad 6.
                    }
                }
            }
        }

        //Si no es modo infinito, se ajusta lo siguiente:
        else {
            if(st.aparicionEnemigo < st.grupo){
                //Se cumple si la aparicion de enemigos es menor que grupo que es el limite al ser modo finito.
                if(st.siguienteAparicionRetardo > 0) {
                    st.siguienteAparicionRetardo--;
                    //al llegar a 0 siguienteAparicionRetardo:
                    if(st.siguienteAparicionRetardo == 0) {
                        //llama la funcion de aparicion de grupo y recibe como parametro cantidadGrupo (bandera establecida) para generar enemigos.
                        aparicionGrupo(st.cantidadGrupo);
                        //Incrementa la aparicion de enemigos
                        st.aparicionEnemigo++;
                    }
                }
                else {
                    bool modo = false; for(bool a:st.enemigoEstado) if(a){ modo = true; break;}
                    if (!modo) st.siguienteAparicionRetardo = st.retraseDeGrupo;
                }
            }
        }

        /**Dirige todas las balas de jugador hacia arriba */

        //Recorre el vector de balaY.
        for (size_t i = 0; i < st.balaY.size();) {
            st.balaY[i]--; //Decrementa balaY para que suba una fila
            //Al llegar al marco superior, se elimina en X y Y. 
            if(st.balaY[i] < 0) {st.balaY.erase(st.balaY.begin() + i); st.balaX.erase(st.balaX.begin() + i); }
            else i++;
        }

        /**Dirige todas las balas enemigas hacia abajo. */
        for (size_t i = 0; i < st.balaEnemigoY.size();){
            st.balaEnemigoY[i]++;
            if(st.balaEnemigoY[i] >= height){ st.balaEnemigoY.erase(st.balaEnemigoY.begin() + i); st.balaEnemigoX.erase(st.balaEnemigoX.begin() + i); }
            else i++;
        }

        /**Disparo del jugador */
        if(st.disparar && (int)st.balaY.size() < st.limiteBalas) {
            st.balaX.push_back(st.posicionX); st.balaY.push_back(st.posicionY-1);
            st.disparar = false;
        }
        disparoEnemigo();

        /**Movimiento del enemigo */
        static int retrasoEnemigo = 0; retrasoEnemigo++; //Contador estatico que aumenta cada cuadro

        //si retrasoEnemigo es mayor que la velocidadEnemigo, los enemigos se mueven en horizontal
        if(retrasoEnemigo > st.velocidadEnemigo) {

            //bandera flip.
            bool flip = false;
            for(size_t i = 0; i < st.enemigoX.size(); i++) {
                if(!st.enemigoEstado[i]) continue;
                //Los enemigos avanzan en la direccion de st.direccionEnemigo.
                st.enemigoX[i] += st.direccionEnemigo;
                //Si un enemigo toca el borde se activa la bandera flip para indicar que tiene que cambiar de direccion.
                if(st.enemigoX[i] <= 0 || st.enemigoX[i] >= width-1) flip = true;
            }

            //si flip == true, se ivnierte la direccion horizontal, st.direccionEnemigo *= -1
            if(flip) {
                st.direccionEnemigo*=-1;
                //todos los enemigos bajan una fila
                for(size_t i = 0; i < st.enemigoY.size(); i++) {
                    if(!st.enemigoEstado[i]) continue;
                    st.enemigoY[i]++;
                    //Si un enemigo toca el marco inferior, el jugador pierde una vida, y se marca ese enemigo como inactivo.
                    if(st.enemigoY[i] >= height-1){
                        st.vidas--; st.enemigoEstado[i] = false;
                        if(st.vidas <= 0) st.gameover = true;
                    }
                }
            }
            //Se reinicia el contador retrasoEnemigo para volver a contar.
            retrasoEnemigo = 0;
        }

        /**Colisiones de bala con enemigo */

        //verificacion de oleada previa
        //inicialmente rondaterminada = true, se vuelve false si hay un enemigo activo.
        bool rondaterminada = true; for (bool a:st.enemigoEstado) if(a){ rondaterminada = false; break;}

        //Recorre las balas (balaX y balaY)
        for (size_t i = 0; i < st.balaY.size();) {
            bool used = false;
            //Compara las coordenas con cada enemigo activo
            for (size_t j = 0; j < st.enemigoY.size(); j++){

                //si la balaX coincide con enemigoX (posicion) al igual que verifica en Y. el estado de enemigo pasa a inactivo (false) y se suma 10 puntos por cada enemigo inactivo a punteo.
                if(st.enemigoEstado[j] && st.balaX[i] == st.enemigoX[j] && st.balaY[i] == st.enemigoY[j]){
                    st.enemigoEstado[j] = false; st.enemigos++; st.punteo += 10;
                    st.balaY.erase(st.balaY.begin() + i); st.balaX.erase(st.balaX.begin() + i);
                    used = true; break;
                }
            }

            //El indice i sigue avanzando si no impacta con nada.
            if (!used) i++;
        }

        //Configuracion modo infinito
        if(st.modoInfinito) {

            //Verifica que no haya enemigos o partidas anteriores activos
            bool modo = false; for(bool a:st.enemigoEstado) if(a){ modo = true; break;}

            //Incrementa la oleada y reinia el temporizador para generar el nuevo grupo de enemigos.
            if(!modo && !rondaterminada) { st.ronda++; st.siguienteAparicionRetardo = st.retraseDeGrupo; }

        //Modo finito
        } else {

            //Si la cantidad de enemigos destruidos es mayor o igual que objetivoEnemigo cambia las banderas st.gamewin y st.gameover a true para confirmar victoria.
            if(st.enemigos >= st.objetivoEnemigo) { st.gamewin = true; st.gameover = true; }
        }

        /**Colision de bala enemiga con jugador */
        //Recorre las balas enemigas en Y
        for (size_t i = 0; i < st.balaEnemigoY.size();) {
            //Compara las coordenas de la balaEnemigoX y Y con la posicion del jugador en X y Y.
            if (st.balaEnemigoX[i] == st.posicionX && st.balaEnemigoY[i] == st.posicionY) {
                //Borra las balas enemigas del vector.
                st.balaEnemigoY.erase(st.balaEnemigoY.begin() + i);
                st.balaEnemigoX.erase(st.balaEnemigoX.begin() + i);
                //Reduce las vidas del jugador con st.vidas, si es menor o igual a 0, cambia la bandera de st.gameover a true.
                st.vidas--; if(st.vidas <= 0) st.gameover = true;
            }
            //Las balas enemigas siguen avanzando si no impactan con nada.
            else i++;
        }

        /**Colision de enemigo con jugador */
        //Recorre la posicion del enemigo en Y
        for (size_t i = 0; i < st.enemigoY.size(); i++){
            //Compara la posicion del enemigo con la del jugador en X y Y
            if (st.enemigoEstado[i] && st.enemigoX[i] == st.posicionX && st.enemigoY[i] == st.posicionY) {
                //st.enemigoEstado es false, y la bandera de st.vidas se reduce. Si st.vidas llega a 0, la bandera de st.gameover se vuelve true y termina el juego.
                st.enemigoEstado[i] = false; st.vidas--; if (st.vidas <= 0) st.gameover = true;
            }
        }

        //Limites del jugador en x
        //Usa la posicion X del jugador y la compara con 0 indicando que no topa con nada. Al ser mayor o igual que width. la posicion X del jugador se iguala a width-1, rebotando con el contorno del juego.
        if (st.posicionX < 0) st.posicionX = 0; if (st.posicionX >= width) st.posicionX = width-1;
    };

    /**Aplicacion de pthread */

    //Se aplica Mutex para asegurar que se aplique bien la funcion de reiniciar() y quen o interfieran otros hilos.
    pthread_mutex_lock(&st.mtx); reiniciar(); pthread_mutex_unlock(&st.mtx);
    //Preparan la terminal para el juego interactivo en tiempo real.
    enableRaw(); hideCursor();

    //struct para agrupar el estado y punteros como GameState, logica y dibujo.
    struct Ctx {
        GameState* st; //Puntero del estado actual del juego
        decltype(logica)* logicaFn; //Puntero a la logica del juego
        decltype(dibujo)* dibujoFn; //Puntero a la renderizacion del juego o dibujo.
    } ctx {&st, &logica, &dibujo};

    //Configura las teclas en tiempo real con mutex y read.
    //Se castea 'arg' para el punteto GameState st.
    auto inputThreadFn = +[](void* arg) -> void* {
        GameState* st = (GameState*)arg;
        while(true){
            //Lectura de un caracter de la entrada normal
            char ch; ssize_t n = read(STDIN_FILENO, &ch, 1);

            //Al leer un caracter
            if (n == 1) {
                //bloquea el mutex
                pthread_mutex_lock(&st -> mtx);

                //Actualiza el estado segun indica la tecla
                if (ch == 'a' || ch == 'A') st -> posicionX--; //a o A para mover a la izquierda
                else if (ch == 'd' || ch == 'D') st -> posicionX++; //d o D para mover a la derecha
                else if (ch == 'q' || ch == 'Q') st -> gameover = true; //q o Q para terminar juego por parte del usuario.
                else if (ch == ' ') st -> disparar = true;

                //Desbloquea el mutex
                pthread_mutex_unlock(&st -> mtx);
            }

            //Bloquea el mutex para verificar si st -> gameover, si se cumple, sale del bucle.
            pthread_mutex_lock(&st -> mtx); bool end = st -> gameover; pthread_mutex_unlock(&st -> mtx);
            // FIX: salir del hilo de entrada si el juego terminó para que pthread_join no se bloquee
            if (end) break;
            //Espera 10 ms para la siguiente iteracion, reduciendo el consumo de la CPU.
            usleep(10000);
        }
        return nullptr;
    };

    //Corre el ciclo principal del juego en un hilo.
    auto gameThreadFn = +[](void* arg) -> void* {
        //Convierte el argumento 'arg' en un puntero a 'Ctx' que tiene st, logicaFn y dibujoFn.
        Ctx* c = (Ctx*)arg;
        GameState* st = c -> st;
        auto& logica = *c -> logicaFn;
        auto& dibujo = *c -> dibujoFn;

        //Define los FPS del juego, siendo 20 FPS
        const int frame_ms = 50;

        while(true){
            //Bloquea el mutex de st.
            pthread_mutex_lock(&st -> mtx);
            //Evalua si el juego termino.
            bool end = st -> gameover;
            //Si el juego no ha terminado, llama la funcion logica(), actualizando las colisiones o polizones.
            if(!end) logica();
            //Desbloquea el mutex
            pthread_mutex_unlock(&st -> mtx);
            //Llama a la funcion dibujo() para renderizar la pantalla.
            dibujo();
            //Si no se espera el bucle.
            if(end) break;
            //esepra frame_ms*1000 para el siguiente ciclo.
            usleep(frame_ms*1000);
        }
        return nullptr;
    };

    //thIn maneja la entrada del jugador, thGame ejecuta la logica y dibujo del juego.

    //lanza cada hilo en paralelo con la funcion indicada.
    pthread_t thIn, thGame;
    pthread_create(&thIn, nullptr, inputThreadFn, &st);
    pthread_create(&thGame, nullptr, gameThreadFn, &ctx);

    //Bloqueo hasta que el hilo termine su partido.
    pthread_join(thGame, nullptr);
    pthread_join(thIn, nullptr);

    //Devuelve la terminal a su estado normal
    showCursor(); disableRaw();
    //limpia los restos graficos
    clearScreen();

    //Guarda los segundos jugados para usarlos en estadistica o puntaje.
    int elapsed = (int)(time(nullptr)-st.inicio);

    //output del resumen final de  una partida
    if(st.gamewin) cout << ANSI_BOLD << ANSI_GREEN << "Ganaste" << ANSI_RESET << "\n";
    else           cout << ANSI_BOLD << ANSI_RED   << "Perdiste" << ANSI_RESET << "\n";

    cout << ANSI_BOLD << "Puntaje:" << ANSI_RESET << " " << ANSI_YELLOW << st.punteo << ANSI_RESET << "\n";
    if(st.modoInfinito)  cout << ANSI_BOLD << "Ronda:" << ANSI_RESET << " " << ANSI_MAGENTA << st.ronda << ANSI_RESET << "\n";
    else                 cout << ANSI_BOLD << "Enemigos destruidos:" << ANSI_RESET << " " << ANSI_MAGENTA << st.enemigos << ANSI_RESET << "/" << st.objetivoEnemigo << "\n";
    cout << ANSI_BOLD << "Tiempo:" << ANSI_RESET << " " << elapsed << "s \n\n";

    //Guardar puntaje
    cout << "Ingrese su nombre para registrar el puntaje (Presiona ENTER para omitir): ";
    string nombre; getline(cin, nombre);
    if (!nombre.empty()) {
        int record = st.modoInfinito ? st.ronda : st.ronda;
        guardarPuntaje(nombre, st.punteo, record, elapsed);
        cout << ANSI_GREEN << "Puntaje guardado en " << rutaPuntaje() << ANSI_RESET << "\n";
    } else {
        cout << ANSI_YELLOW << "Puntaje no guardado" << ANSI_RESET << "\n";
    }

    cout << "\n Presiona ENTER para continuar";
    string _; getline(cin, _);
}
