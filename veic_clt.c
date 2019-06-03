#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <netdb.h>


#define VEHIC_MAXPLEN 81

static const char uri[] = "/~jes/sistc/pl/ws/vehicle";
static const char uriJson[] = "/~jes/sistc/pl/ws/vehicles.php?o=JSON";
static const char servername[] = "ave.dee.isep.ipp.pt";

typedef struct {
  char plate[6];
  char owner[VEHIC_MAXPLEN];
  double value;
} vehic_t;

void vehic_read(vehic_t *v);
void vehic_print(vehic_t *v);
int vehic_menu();
int parse_json(char c, vehic_t *v, char *buffer, int maxlen, int *nchars, int *state);  
int my_connect(char *servername, char *port);
void submeter_dados_get();


int main (int argc, char* const argv[]) {
  int op;

  vehic_t v;
      
  while(1) {
    op = vehic_menu();
    switch(op) {
    case 0: exit(1);
    case 1:
      vehic_read(&v);

      //send new record to server
      //...
            
      break;
    case 2:   
      //call vehic_print ( e.g., vehic_print(&v); ) for each record 
      //...
      submeter_dados_get();
      
      break;
    } 
  }
  return 0;
}



//do not change code below!!

int parse_json(
  char c, //should contain received character
  vehic_t *v,    //output parameter: should point to a vehic_t storage location
  char *buffer, //it must able to hold at least VEHIC_MAXPLEN-1 chars
  int maxlen,    //buffer size
  int *nchars,   // should point to a int storage location
  int *state)    // should point to a int storage location initialized with 0
{
      switch(*state) {
      case 0:
      case 2:
      case 4:
        if( c == '"' ) { //found opening "
          (*state)++;
          *nchars = 0;
          break;
        }
        
        if( c == '['  && *state != 0 ) { //new record before completing current one
          printf("invalid record\n");
          *state = 0;
          break;
        }
        
        break;
      case 1://reading plate
        if( c == '"' ) {  //found closing "
          *state = 2;
          break;
        }
        
        if( *nchars == 6 ) //discard unexpected chars
          break;

        v->plate[(*nchars)++] = c;      
        break;
      case 3://reading owner
        if( c == '"' ) { //found closing "
          *state = 4;
          v->owner[*nchars] = 0;
          break;
        }

        if( *nchars == VEHIC_MAXPLEN-1 ) //discard unexpected chars
          break;
        
        v->owner[(*nchars)++] = c;                
        break;
      case 5://reading value 
        if( c == '"' ) { //found closing "
          *state = 0;
          
          buffer[*nchars] = 0;
          v->value = atof(buffer);
          //vehic_print(&v);
          return 1; //means it got a new vehic_t record
          break;
        }
        
        if( *nchars == maxlen ) //discard chars if string would exceed buffer size
          break;

        buffer[(*nchars)++] = c;                
        break;
      }
      return 0;
}


int my_connect(char *servername, char *port) {

  //get server address using getaddrinfo
  struct addrinfo hints;
  struct addrinfo *addrs;
  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_family = AF_INET;
  int r = getaddrinfo(servername, port, &hints, &addrs);
  if (r != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(r));
    exit(1);
  }
  
  //create socket
  int s = socket(AF_INET, SOCK_STREAM, 0);
  if (s == -1) {
    perror("socket");  
    exit(2);
  }
  
  //connect to server
  r = connect(s, addrs->ai_addr, addrs->ai_addrlen);
  if (r == -1) {
    perror("connect");  
    close(s);
    exit(3);
  }
  
  return s;
}


int vehic_menu() { 
  int op; 
  printf("\n0 - Exit\n"); 
  printf("1 - Insert new record\n"); 
  printf("2 - Print all\n"); 
  printf("\nEnter option: "); 
  fflush(stdout);
  scanf("%d", &op); 
  getchar(); //consume '\n'
  printf("\n");
  
  return op; 
}


void vehic_read(vehic_t *v) {
	
  char buf[VEHIC_MAXPLEN];
  char params[4096];
  char args[4096];
  
  
  int server_socket = my_connect(servername, "80");
  
  printf("Plate (6 characters): ");
  fgets(buf, VEHIC_MAXPLEN, stdin);
  memcpy(v->plate, buf, 6);
  printf("Owner: ");
  fgets(v->owner, VEHIC_MAXPLEN, stdin);
  v->owner[strlen(v->owner)-1] = 0; //"move" the end of the string to the place of '\n'
  printf("Value: ");
  scanf("%lf", &v->value);
  getchar(); //consume \n same as fflush(stdin)
  
  sprintf(params, "plate=%.6s&owner=%s&value=%.2lf", v->plate, v->owner, v->value);
  int n=sprintf(args,"POST %s HTTP/1.1\r\n""Host: %s \r\n""Content-Type: application/x-www-form-urlencoded\r\n"
		"Content-Length: %ld\r\n""Connection: close\r\n""\r\n""%s",uri,servername,strlen(params),params);
		
  write(server_socket, args, strlen(args));
  
  while(1){
  int n = read(server_socket, buf, 1);
    if(n <= 0)
      break;
    
    putchar(buf[0]);
    fflush(stdout);
    if(buf[0]=='\n')
      printf(": ");
  }
}

void vehic_print(vehic_t *v) {
  v->owner[VEHIC_MAXPLEN-1] = 0;//double check
  //printf("%.6s - %s - %.2lf\n", v->plate, v->owner, v->value);
  printf("%.6s - %-50s - %14.2lf\n", v->plate, v->owner, v->value);
}

void submeter_dados_get()
{
    int sd = my_connect(servername, "80");
    
    char HTTP_get[4096];
    int n=sprintf(HTTP_get,"GET %s HTTP/1.1\r\n""Host: %s \r\n\r\n",uriJson,servername);
    

    FILE *fp; //stream para o socket e enviar o pedido GET
    fp=fdopen(sd,"w+");
    write(sd,HTTP_get,n);

    char buffer[4000]; // Variaveis para tratar da parte JSON
    int sin=0;
    int nchars;
    int state = 0;
    vehic_t v;
    char aux[256];
    char c;

        while(fgets(buffer, sizeof(buffer), fp)!=NULL)     {
      if(strcmp(buffer, "\r\n")==0)
       {
        sin=1;
        break;
       }
      
    }
     
   

   if(sin==1)    {
      while(fread(&c,sizeof(char),1,fp)!=0)       {
        if( parse_json(c, &v, aux, sizeof(aux), &nchars, &state) == 1 )
        vehic_print(&v); //Sempre que a funcao detetar um registo, imprime.

      }
      
   }
   fclose(fp);
}


