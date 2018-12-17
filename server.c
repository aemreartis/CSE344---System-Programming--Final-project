#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include<math.h>
#include <pthread.h>
#include<signal.h>

// Struct to keep clients information
typedef struct clientInfo{
	char name[50];
	char priority[1];
	int homework;
	int jobTime;
	
}clientInfo;

//Struct to keep providers information
typedef struct providerInfo{
	char name[50];
	clientInfo clientQueue[2];
	int numOfClient;
	int isProviderActive;
	int performance;
	int price;
	int duration;
	int activeTime;
	int taskNum;
}providerInfo;


providerInfo* providers;
pthread_mutex_t mutex;
int numOfProviders=1;
int fdThreads[2];
FILE* logFile;
int sockfd, newsockfd, portno;
 pthread_t handleThId, *providersThId;

//Signal handler
void sig_handler(int signo);

//When a message recived this fucntion sends it to proper provider
static void* handle_accept(void* arg);

//The provider threads waits for a job to add their list , if list is not empty, it gets the job and sends its result to proper client   
static void* thread_provider(void* arg);

//Prints error and exit 
void error(const char *msg);

//Calculate Cos (Providers message)
double calculateCos(double degree);

//calculates factorial
int factorial(int n);

int main(int argc, char *argv[])
{

     socklen_t clilen;
     char buffer[256]="";
     struct sockaddr_in serv_addr, cli_addr;
     int n=0,s=0,i=0,size=0;
     /*read and fill variables*/
 	 FILE* dataFile=NULL;
     int ret=0;
	 char *line = (char *) malloc(100 * sizeof(char));
     char* token=NULL;
     char tokenList[]=" \n";
     
     providers=(providerInfo*) malloc(sizeof(providerInfo));
	 logFile=fopen(argv[3],"w");
     
     /*PRINT USAGE*/
     if (argc != 4) {
         fprintf(stderr,"ERROR, no port provided\n");
          fprintf(logFile,"ERROR, no port provided\n");
         fprintf(stderr,"USAGE: ./homeworkServer 5555 data.dat log.dat \n");
          fprintf(logFile,"USAGE: ./homeworkServer 5555 data.dat log.dat \n");
         exit(1);
     }
     if (signal(SIGINT, sig_handler) == SIG_ERR){
  		fprintf(stderr,"\ncan't catch SIGINT\n");
  		fprintf(logFile,"\ncan't catch SIGINT\n");
  		}
     if (signal(SIGTERM, sig_handler) == SIG_ERR){
  		fprintf(stderr,"\ncan't catch SIGINT\n");
  		fprintf(logFile,"\ncan't catch SIGINT\n");
	}
	srand(time(0));		
	pipe (fdThreads);
	fprintf(stderr,"Logs keep at %s",argv[3]);
	fprintf(logFile,"Logs keep at %s",argv[3]);
     /*READ PROVIDERS AND FILL THEIR INFORMATION*/
		 dataFile=fopen(argv[2],"r");


		 s=getline(&line,&size,dataFile);
		 do{
				s=getline(&line,&size,dataFile);
				if(s!=EOF){
					token = strtok(line, tokenList);
					strcpy(providers[i].name,token);
					
					token = strtok(NULL, tokenList);
					providers[i].performance=atoi(token);
				
					token = strtok(NULL, tokenList);
					providers[i].price=atoi(token);
				
					token = strtok(NULL, tokenList);
					providers[i].duration=atoi(token);
					providers=(providerInfo*) realloc(providers,++numOfProviders*sizeof(providerInfo));
					providers[i].numOfClient=0;
					providers[i].isProviderActive=1;
					providers[i].activeTime=0;
					providers[i].clientQueue[0].jobTime=0;
					providers[i].clientQueue[1].jobTime=0;
					providers[i].taskNum=1;
					i++;
				}
		}while(s!=EOF );
	free(line);
	fclose(dataFile);
	dataFile=NULL;
	/*MUTEX INITIALIZE*/ 
		pthread_mutex_init (&mutex, NULL);	
     /*CREATE THREAD POOL FOR PROVIDERS*/
     providersThId= (pthread_t*) malloc(numOfProviders*sizeof(pthread_t));
     fprintf(stderr,"%d provider threads created\nName  \tPerformance \tPrice  Duration\n",numOfProviders-1);
     fprintf(logFile,"%d provider threads created\nName  \tPerformance \tPrice  Duration\n",numOfProviders-1);
	for(i=0;i<numOfProviders-1;i++){
     	fprintf(stderr,"%s \t %d\t\t%d\t   %d\n",providers[i].name,providers[i].performance,providers[i].price,providers[i].duration);
     	fprintf(logFile,"%s \t %d\t\t%d\t   %d\n",providers[i].name,providers[i].performance,providers[i].price,providers[i].duration);
     }
     for(i=0;i<numOfProviders-1;i++){
     	pthread_create(&(providersThId[i]),NULL,thread_provider,(void*)i);
     	fprintf(stderr,"Provider %s waiting for tasks.\n",providers[i].name);
     	fprintf(logFile,"Provider %s waiting for tasks.\n",providers[i].name);
     }

     
     /*OPEN SOCKET AND ERROR CHECK*/
     sockfd = socket(AF_INET, SOCK_STREAM, 0);     
     if (sockfd < 0)
        error("ERROR opening socket");
     
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     
     /*BIND*/
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     /*LISTEN*/
     listen(sockfd,5);
     
     clilen = sizeof(cli_addr);
      fprintf(stderr,"Server is waiting for client connections at port %s\n",argv[1]);	
 	  fprintf(logFile,"Server is waiting for client connections at port %s\n",argv[1]);		
     while(1){
		 /*ACCEPT*/
		 newsockfd = accept(sockfd,(struct sockaddr *)&cli_addr,&clilen);                     
		 if (newsockfd < 0) 
		      error("ERROR on accept");
		 
		 s= pthread_create(&handleThId,NULL,handle_accept,(void*)newsockfd);
		 if(s!=0){
			fprintf(stderr,"create error\n");
			fprintf(logFile,"create error\n");
			return 0;
		 }
     }
	for(i=0;i<numOfProviders;i++)
	pthread_join(providersThId[i],(void*)&ret);
    free(providersThId);
	free(providers);
	close(newsockfd);
	close(sockfd);
	fclose(logFile);
	providersThId=NULL;
	providers=NULL;
	logFile=NULL;
     return 0; 
}



//PROVIDER 
static void* thread_provider(void* arg){

	int nth =(int) arg;
	double jobTime=0;
	int prepareTime,i,j,n;
	double hwResult;
	char result[256]="";

	/*LOOP1:WORKS UNTIL TIME IS UP*/
	while(providers[nth].duration>providers[nth].activeTime){

			while(providers[nth].numOfClient==0&&providers[nth].duration>providers[nth].activeTime){}
			
			fprintf(stderr,"Provider %s is processing task number %d: %d\n",
			providers[nth].name,providers[nth].taskNum,providers[nth].clientQueue[0].homework);
			
			fprintf(logFile,"Provider %s is processing task number %d: %d\n",
			providers[nth].name,providers[nth].taskNum,providers[nth].clientQueue[0].homework);
			
			jobTime=rand()%11+5;
			
			/*DO THE HW */
			hwResult=calculateCos(providers[nth].clientQueue[0].homework);
			
			/*SLEEP FOR SIMULATION*/
			sleep(jobTime);
			
			fprintf(stderr,"Provider %s completed task number %d: cos(%d)=%.2lf in %.2lf seconds.\n" ,
			providers[nth].name,
			providers[nth].taskNum,providers[nth].clientQueue[0].homework,
			hwResult,jobTime);
			
			fprintf(logFile,"Provider %s completed task number %d: cos(%d)=%.2lf in %.2lf seconds.\n" ,
			providers[nth].name,
			++providers[nth].taskNum,providers[nth].clientQueue[0].homework,
			hwResult,jobTime);
			
			/*Mutex lock*/
			pthread_mutex_lock(&mutex);
			
			/*EDIT QUEUE */
			if(providers[nth].numOfClient==2){
				strcpy(providers[nth].clientQueue[0].name,providers[nth].clientQueue[1].name);
				strcpy(providers[nth].clientQueue[0].priority,providers[nth].clientQueue[1].priority);
				providers[nth].clientQueue[0].homework=providers[nth].clientQueue[1].homework;
			}
			providers[nth].numOfClient--;
			
			/*Mutex unlock*/
			pthread_mutex_unlock(&mutex);
			
			/*SEND HW RESULT TO Handle*/
			sprintf(result,
			"%s's task completed by %s in %.2lf seconds.cos(%d)=%.2lf, cost is %d TL,total time spend %.2lf seconds\n",
			providers[nth].clientQueue[0].name,providers[nth].name,jobTime,
			providers[nth].clientQueue[0].homework,hwResult,providers[nth].price,jobTime);
			
			n = write(fdThreads[1],result,sizeof(result));
	 
			if (n < 0) error("ERROR writing to handler thread");
			/*TIME UPDATE*/
			providers[nth].activeTime+=jobTime;

	}		
	sprintf(result,"%s's time is up for today.\n ",providers[nth].name);
	n = write(fdThreads[1],result,sizeof(result));
	providers[nth].isProviderActive=0;
	
	return (void*) nth;
}

/*ACCEPT HANDLE*/
static void* handle_accept(void* arg){

	int fd =(int) arg;
	int prepareTime,i,j,n;
	char buffer[256];
	char result[256];
	bzero(buffer,256);
	char* token ;
	char tokenList[]=" \n";	 
	char priority[1];
	char name[50];
	char noProvider[]="NO PROVIDER IS AVALIABLE";
	int homework,tempIndex=-1,tempValCost=99999,tempValPerform=0,tempValTime=0;
	
	/*READ MESSAGE FROM CLIENT*/
	n = read(fd,buffer,255);     
	if (n < 0) error("ERROR reading from socket");

	/*PARSE INPUT*/
	token = strtok(buffer, tokenList);
	strcpy(name,token);

	token = strtok(NULL, tokenList);
	strcpy(priority,token);

	token = strtok(NULL, tokenList);
	homework=atoi(token);
	
	/*MUTEX LOCK */
	pthread_mutex_lock(&mutex);
	
	/*FIND PROVIDER AND POST THE JOB*/
	if(strcmp(priority,"C")==0){
		for(i=0;i<numOfProviders-1;i++)
			if(providers[i].numOfClient<2 && providers[i].price<tempValCost&&providers[i].isProviderActive){
				tempIndex=i;
				tempValCost=providers[i].price;
			}
	}		
	else if (strcmp(priority,"Q")==0){
		for(i=0;i<numOfProviders-1;i++)
			if(providers[i].numOfClient<2 && providers[i].performance>tempValPerform &&providers[i].isProviderActive){
				tempIndex=i;
				tempValPerform=providers[i].performance;
			}
	}
	else if (strcmp(priority,"T")==0){
		for(i=0;i<numOfProviders-1;i++)
			if(providers[i].numOfClient==0 && providers[i].duration>tempValTime&&providers[i].isProviderActive){
				tempIndex=i;
				tempValTime=providers[i].duration;
			}
		if(tempIndex==-1)	
			for(i=0;i<numOfProviders-1;i++)
				if(providers[i].numOfClient==1 && providers[i].duration>tempValTime&&providers[i].isProviderActive){
					tempIndex=i;
					tempValTime=providers[i].duration;
				}
	}		
	else
		error("ERROR PRIORITY");
	
	if(tempIndex!=-1){
		fprintf(stderr,"%d--%s\n",tempIndex,providers[tempIndex].name);		
		strcpy(providers[tempIndex].clientQueue[providers[tempIndex].numOfClient].name,name);	
		strcpy(providers[tempIndex].clientQueue[providers[tempIndex].numOfClient].priority,priority);	
		providers[tempIndex].clientQueue[providers[tempIndex].numOfClient].homework=homework;
		providers[tempIndex].numOfClient++;
		
		fprintf(stderr,"Client %s (%s %d) connected, forwarded to provider %s \n",name,priority,homework,providers[tempIndex].name);
		fprintf(logFile,"Client %s (%s %d) connected, forwarded to provider %s \n",name,priority,homework,providers[tempIndex].name);		
	}
	else{
		perror(noProvider);
	 	strcpy(result,noProvider);
 	}
	 /*MUTEX UNLOCK*/
	 pthread_mutex_unlock(&mutex);
	 /*WAIT FOR RESULT*/
	 if(tempIndex!=-1){
	 	n=read(fdThreads[0],result,sizeof(result));
	 	if (n < 0) error("ERROR readingfrom provider");
	 }
	 
	/*SEND HW RESULT BACK TO CLIENT*/
	n = write(fd,result,sizeof(result));
	if (n < 0) error("ERROR writing to socket");
	 
	return (void*) fd;
}
void error(const char *msg)
{
    perror(msg);
    exit(1);
}

double calculateCos(double degree){
	double i;
	double result=0;

	for(i=0;i<5;i+=1){	
		result+=(pow(-1,i)/factorial(2*i))*pow((degree*3.14/180.0),2*i);
	}
	return result;
}

int factorial(int n)
{
	if(n == 1 || n == 0)
  		return 1 ;
  	else
  		return factorial(n - 1) * n;
}

void sig_handler(int signo)
{
 	if (signo == SIGINT||signo == SIGTERM){
 		int i=0,ret;
		fprintf(stderr,"\nTermination signal recieved.\nTerminating all clients \nTerminating all providers \nStatistics\nName \tNumber of clients served \n");
		fprintf(logFile,"\nTermination signal recieved.\nTerminating all clients \nTerminating all providers \nStatistics\nName \tNumber of clients served \n");
		
		for (i=0;i<numOfProviders-1;i++){
			fprintf(stderr,"%s \t %d\n",providers[i].name,providers[i].taskNum-1);
			fprintf(logFile,"%s \t %d\n",providers[i].name,providers[i].taskNum-1);
		}
		
		for (i=0;i<numOfProviders;i++)
			pthread_detach(providersThId[i]);
		free(providersThId);
		free(providers);
		close(newsockfd);
		close(sockfd);
		fclose(logFile);
		providersThId=NULL;
		logFile=NULL;

		pthread_mutex_destroy(&mutex);
		exit(1);
	}
}
