/**
 * @file coffeemaker.h
 *
 * @author Ulrike Schaefer 1327450
 *
 * 
 * @brief the coffeemaker is a server-client architecture where the client can order a coffee and the server responds if the coffee can be made and if so how long it will take.
 *
 * @date 01.04.2017
 * 
 */


#ifdef ENDEBUG
#define DEBUG(...) do { fprintf(stderr, __VA_ARGS__); } while(0)
#else
#define DEBUG(...)
#endif


 /**
 * @brief Length of an array
 */
#define COUNT_OF(x) (sizeof(x)/sizeof(x[0]))


 /**
 * @brief Default Portnumber to connect to
 */
char *portno = "1821";

/**
 * @brief enum of existing coffee-flavors (up to 32 are allowed)
 */
enum { Kazaar, Dharkan, Roma, Livanto, Volluto, Cosi, Cappricio, Appregio, Caramelito, Vanilio, Ciocattino };

/**
 * @brief the different coffee flavors the coffemaker currently can make
 */
char* coffeeNames[] = { "Kazaar", "Dharkan", "Roma", "Livanto", "Volluto", "Cosi", "Cappricio", "Appregio", "Caramelito", "Vanilio", "Ciocattino" };