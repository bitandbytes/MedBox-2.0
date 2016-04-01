#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "wiringPi.h"
#include "MQTTClient.h"
#include "MQTTClientPersistence.h"
#include "time.h"
//MQTT definitions
/*To get only the last message retain flag shouw be set when publishing, also the QOS should be set to 0*/
#define QOS         2
#define TIMEOUT     10000L

//wiringPi definitions
#define LED_1           02
#define LED_2           03
#define LED_3           04
#define SWITCH_1        21
#define SWITCH_2        13
#define SWITCH_3        16
#define TRUE            1
#define FALSE           0
#define debounceDelay	50
#define interruptDelay  10 
#define LOCK_KEY	0
int boxStatus_1 = 0;
int boxStatus_2 = 0;
int boxStatus_3 = 0;
int threadFlag = 0;

//Queue structure
typedef struct Queue
{
        int capacity;
        int size;
        int front;
        int rear;
        int *elements;
}Queue;
Queue *Q;

FILE *dataStream;

int count = 1; //Only for debuging purposes

//Function Prototypes
void connlost(void*, char*);
void delivered(void *context, MQTTClient_deliveryToken dt);
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message);
void connlost(void *context, char *cause);

volatile MQTTClient_deliveryToken deliveredtoken;

Queue * createQueue(int maxElements)
{
        /* Create a Queue */
        Queue *Q;
        Q = (Queue *)malloc(sizeof(Queue));
        /* Initialise its properties */
        Q->elements = (int *)malloc(sizeof(int)*maxElements);
        Q->size = 0;
        Q->capacity = maxElements;
        Q->front = 0;
        Q->rear = -1;
        /* Return the pointer */
        return Q;
}

void dequeue(Queue *Q)
{
        /* If Queue size is zero then it is empty. So we cannot pop */
        if(Q->size==0)
        {
                printf("Queue is Empty\n");
                return;
        }
        /* Removing an element is equivalent to incrementing index of front by one */
        else
        {
                Q->size--;
                Q->front++;
                /* As we fill elements in circular fashion */
                if(Q->front==Q->capacity)
                {
                        Q->front=0;
                }
        }
        return;
}

int front(Queue *Q)
{
        if(Q->size==0)
        {
                printf("Queue is Empty\n");
                exit(0);
        }
        /* Return the element which is at the front*/
        return Q->elements[Q->front];
}

void enqueue(Queue *Q,int element)
{
        /* If the Queue is full, we cannot push an element into it as there is no space for it.*/
        if(Q->size == Q->capacity)
        {
                printf("Queue is Full\n");
        }
        else
        {
                Q->size++;
                Q->rear = Q->rear + 1;
                /* As we fill the queue in circular fashion */
                if(Q->rear == Q->capacity)
                {
                        Q->rear = 0;
                }
                /* Insert the element in its rear side */ 
                Q->elements[Q->rear] = element;
        }
        return;
}

void interruptFunc_1(void){
	printf("$Time: %f\n",(float)(clock()/1000000.0F)*1000);
	printf("Interrupt Occured 1\n");
	delay(interruptDelay);				//Level of sensitivity of the interrupt(higher the delay less the sensitiveness)
	boxStatus_1 = digitalRead(SWITCH_1);
	printf("Interrupt End 1\n");
	printf("$Time: %f\n",(float)(clock()/1000000.0F)*1000);
}

void interruptFunc_2(void){
        printf("Interrupt Occured 2\n");
        delay(interruptDelay);                             //Level of sensitivity of the interrupt(higher the delay less$
        boxStatus_2 = digitalRead(SWITCH_2);
        printf("Interrupt End 2\n");
}

void interruptFunc_3(void){
        printf("Interrupt Occured 3\n");
        delay(interruptDelay);                             //Level of sensitivity of the interrupt(higher the delay less$
        boxStatus_3 = digitalRead(SWITCH_3);
        printf("Interrupt End 3\n");
}
void publishBoxStatus(int drawer,int boxStatus){ 
	piLock(LOCK_KEY);
	enqueue(Q,drawer+boxStatus);
	piUnlock(LOCK_KEY);
}

void lightLED(char LED){
	switch(LED){
	case '1' : digitalWrite(LED_1,HIGH); break;
	case '2' : digitalWrite(LED_2,HIGH); break;
	case '3' : digitalWrite(LED_3,HIGH); break;
	case '4' : digitalWrite(LED_1,LOW); break;
        case '5' : digitalWrite(LED_2,LOW); break;
        case '6' : digitalWrite(LED_3,LOW); break;

	default  : printf("Invalid MQTT message\n   message: ");break;
	}
}

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    int i;
    char* payloadptr;
    char messageText[message->payloadlen];

    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");

    payloadptr = message->payload;
    for(i=0; i < message->payloadlen; i++)
    {
     //   putchar(*payloadptr++);
	  messageText[i] = *payloadptr++;
    }
	  messageText[i] = '\0';
    lightLED(messageText[0]);
    printf("%s",messageText);
    putchar('\n');
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

PI_THREAD (mqttStream)
{

/*	//Creating the MQTT stream
 	char *buffer = NULL;
	size_t length = 0;
	off_t eob;
	dataStream = open_memstream(&buffer, &length);
       	if (dataStream == NULL)
       	{
              	printf("MQTT data stream ERROR!\n");
               	exit(-1);
       	}
*/
	//Queue initialization
	Q = createQueue(10);
	//MQTT pub data from the file
	char PAYLOAD[3];
     	char ADDRESS[50];
     	char CLIENTID[23];
     	char TOPIC[80];
     	char PORT[5];
     	FILE* fp = fopen("input_pub.config","r");
     	fscanf(fp,"%s",ADDRESS);
     	fscanf(fp,"%s",CLIENTID);
     	fscanf(fp,"%s",TOPIC);
     	fscanf(fp,"%s",PORT);
     	sprintf(ADDRESS,"%s:%s",ADDRESS,PORT);
     	fclose(fp);

	while(1)
	{
		if(Q->size != 0)
		{
			MQTTClient client;
			MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
 	 		MQTTClient_message pubmsg = MQTTClient_message_initializer;
	     		MQTTClient_deliveryToken token;
     			int rc;

    			 MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_USER, NULL);
    			 conn_opts.keepAliveInterval = 100;
    			 conn_opts.cleansession = 0;

    			 MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

    			 if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    		 	{
        			 printf("Failed to connect, return code %d\n", rc);
        			 exit(-1);
     			 }
			 piLock(LOCK_KEY);
     			 sprintf(PAYLOAD,"%i. %i",count,front(Q));
			 dequeue(Q);
			 count++;
			 piUnlock(LOCK_KEY);
    			 pubmsg.payload = PAYLOAD;
    			 pubmsg.payloadlen = strlen(PAYLOAD);
     			 pubmsg.qos = QOS;
     			 pubmsg.retained = 1;
     			 deliveredtoken = 1;
		printf("$Time: %f\n",(float)(clock()/1000000.0F)*1000);
     			 MQTTClient_publishMessage(client, TOPIC, &pubmsg, &token);
     			 printf("Waiting for publication of %s\n" "on topic %s for client with ClientID: %s\n", PAYLOAD, TOPIC, CLIENTID);
		     //delay(50);
     			 while(deliveredtoken != token);
     			//Not getting a token
		printf("$Time: %f\n",(float)(clock()/1000000.0F)*1000);
     			MQTTClient_disconnect(client, 100);
		printf("$Time: %f\n",(float)(clock()/1000000.0F)*1000);
     			MQTTClient_destroy(&client);
     			printf("Message Sent\n");
		printf("################################################################\n");
		}
	}
}

int main(int argc, char* argv[])
{
    //Getting input from the file
    char ADDRESS[50];
    char CLIENTID[23];
    char TOPIC[80];
    char PORT[5];
    FILE* fp = fopen("input_sub.config","r");
    fscanf(fp,"%s",ADDRESS);
    fscanf(fp,"%s",CLIENTID);
    fscanf(fp,"%s",TOPIC);
    fscanf(fp,"%s",PORT);
    sprintf(ADDRESS,"%s:%s",ADDRESS,PORT);
    fclose(fp);
    //WiringPi setup
    wiringPiSetupGpio ();
    pinMode (LED_1, OUTPUT);
    pinMode (LED_2, OUTPUT);
    pinMode (LED_3, OUTPUT);
    //pinMode (SWITCH_1, INPUT);
    //pinMode (SWITCH_2, INPUT);
    //pinMode (SWITCH_3, INPUT);
    //Pull up input pins
    pullUpDnControl (SWITCH_1, PUD_UP);
    pullUpDnControl (SWITCH_2, PUD_UP);
    pullUpDnControl (SWITCH_3, PUD_UP);
    //Set Interrupts
    wiringPiISR(SWITCH_1,INT_EDGE_BOTH,&interruptFunc_1);
    wiringPiISR(SWITCH_2,INT_EDGE_BOTH,&interruptFunc_2);
    wiringPiISR(SWITCH_3,INT_EDGE_BOTH,&interruptFunc_3);
    //Pull down the non ground pins
    pullUpDnControl (19, PUD_DOWN);
    pullUpDnControl (26, PUD_DOWN);
    //Lights Initialization
    digitalWrite(LED_1,HIGH);
    digitalWrite(LED_2,LOW);
    digitalWrite(LED_3,LOW);
    delay(100);
    digitalWrite(LED_1,LOW);
    digitalWrite(LED_2,HIGH);
    digitalWrite(LED_3,LOW);
    delay(100);
    digitalWrite(LED_1,LOW);
    digitalWrite(LED_2,LOW);
    digitalWrite(LED_3,HIGH);
    delay(100);
    digitalWrite(LED_1, LOW);
    digitalWrite(LED_2, LOW);
    digitalWrite(LED_3, LOW);

    //MQTT Setup
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    int ch;

    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_USER, NULL);
    conn_opts.keepAliveInterval = 100;
    conn_opts.cleansession = 0;

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(-1);
    }
    printf("Subscribing to topic %s\nfor client %s using QoS %d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
    MQTTClient_subscribe(client, TOPIC, QOS);

    //Start the new thread
    if(piThreadCreate(mqttStream) != 0)
    {
	printf("ERROR in creating MQTT streaming thread");
    }

    //Main loop
    int previousBoxStatus_1,previousBoxStatus_2,previousBoxStatus_3;

    previousBoxStatus_1 = boxStatus_1;
    previousBoxStatus_2 = boxStatus_2;
    previousBoxStatus_3 = boxStatus_3;

    do 
    {
	if(previousBoxStatus_1 != boxStatus_1){
		delay(debounceDelay);			//Time delay after an interrupt for the signal to be stable
		publishBoxStatus(10,boxStatus_1);	//TODO try to use a seperate thread to send mqtt message
		previousBoxStatus_1 = boxStatus_1;	//Box open close status is identified correctly, the mqtt message loss is the problem
	}
	if(previousBoxStatus_2 != boxStatus_2){
                delay(debounceDelay);                     //Time delay after an interrupt for the signal to be stable
                publishBoxStatus(20,boxStatus_2);
                previousBoxStatus_2 = boxStatus_2;
        }
	if(previousBoxStatus_3 != boxStatus_3){
                delay(debounceDelay);                     //Time delay after an interrupt for the signal to be stable
                publishBoxStatus(30,boxStatus_3);
                previousBoxStatus_3 = boxStatus_3;
        }

//if the previous mode is not the status now publish else do nothing	
       // ch = getchar();
    } while(ch!='Q' && ch != 'q');

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}

