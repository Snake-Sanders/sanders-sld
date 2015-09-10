#include <iostream>
#include <SDL2/SDL.h>
#include <mosquitto.h>
#include <math.h>
#include <stdlib.h>

// Graphic
const unsigned int GRAPH_BARS_NUM = 10;

// Sensor Patterns
const char* SENSOR_STATE_PATT 	= "recruitment/ciot/state";		//!< Sensor State
const char* SENSOR_VALUE_PATT 	= "recruitment/ciot/sensor1";	//!< Sensor measurement
const char* SENSOR_WAKEUP_PATT 	= "recruitment/ciot/wake";		//!< Sensor trigger wakeup 

// sensor status response
const char* SENSOR_STATUS_SLEEP = "sleep";
const char* SENSOR_STATUS_AWAKE = "awake";

// sensor measurement
typedef struct SensorStatus
{
	float timestamp;//!< Measurement Timestamp 
	float value;	//!< Value measured
	bool  isAwaken;	//!< Sensor current status: awake or sleep

	SensorStatus& operator=(SensorStatus other)
	{
		timestamp = other.timestamp;
		value 	  = other.value;
		isAwaken  = other.isAwaken;

		return *this;
	}

}SensorStatus;

SensorStatus gSensorHistory[GRAPH_BARS_NUM];	//!< Collection of sensor's status 

static unsigned int gCurrentStatusIdx 	= 0; 		//!< Index of the most recent status introduced in the history collection
static unsigned int gHistorySize 	 	= 0;		//!< Capacity of the status history collection
static bool mIsNewMeasurementStored   	= true;	//!< Flag that indicate if there is a new measurement in the history collection


int  main(int, char**);
void log_result(const char * subject, int resultCode);
void message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message);
void connect_callback(struct mosquitto *mosq, void *userdata, int result);
void draw(SDL_Renderer* renderer, float delta_time);
void wakeup_sensor(struct mosquitto *mosq);
void store_new_measurement( SensorStatus & newStatus );
SensorStatus & get_measurement( unsigned int posIndex );
bool is_new_measurement_stored();
int  get_history_measurement_size();

int  get_history_measurement_size()
{
	return gHistorySize;
}

bool is_new_measurement_stored()
{
	bool isThereNewData = mIsNewMeasurementStored;
	mIsNewMeasurementStored = false;

	return isThereNewData;
}

void store_new_measurement( SensorStatus & newStatus)
{
	
	if( gHistorySize < GRAPH_BARS_NUM)
		gHistorySize++;

	// control circular buffer
	if( gCurrentStatusIdx >= GRAPH_BARS_NUM)
	{
		gCurrentStatusIdx = 0;
	}
		
	gSensorHistory[gCurrentStatusIdx] = newStatus;
	gCurrentStatusIdx++;
	mIsNewMeasurementStored = true;
}

SensorStatus & get_measurement( int posIndex )
{
	int targetIdx = 0;

	if( posIndex < 0 || posIndex >= GRAPH_BARS_NUM)
	{
		std::cout << "Error idx not in history" << std::endl;
	}
	else
	{
		// control circular buffer
		targetIdx = (gCurrentStatusIdx -1) - posIndex;

		if( targetIdx < 0 )
		{
			targetIdx = GRAPH_BARS_NUM - abs(targetIdx) ;
		}
	}
	
	std::cout << "---------req:" << posIndex << "t:" << targetIdx << std::endl;
	
	for (int i = 0; i < GRAPH_BARS_NUM; i++)
	{
		const char * idxPointer = (i == gCurrentStatusIdx) ? "<i" : "" ;
		const char * idxTarget  = (i == targetIdx) ? " <t" : "" ;
		std::cout << "id:" << i << ":" << gSensorHistory[i].value << idxPointer << idxTarget << std::endl;				
	}

	//std::cout << gCurrentStatusIdx << "-" << posIndex << "=" << targetIdx << std::endl;

	return gSensorHistory[ targetIdx ];
}

void message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
	SensorStatus sensor;

	if(message->payloadlen > 0 && message->payload != 0)
	{
		if( strcmp( message->topic, SENSOR_VALUE_PATT) == 0 )
		{						
			// message with a new sensor measurement

			unsigned int bufferSize = message->payloadlen;
			char* pCharIdx;
			char buffer[bufferSize];

			// copy the payload to buffer and replace comma by white space
			for( int i = 0; i < bufferSize; i++ )
			{
				buffer[i] = ((const char*)message->payload)[i];

				if( buffer[i] == ',' )
				{
					buffer[i] = ' '; // replace the comma
				} 
			}
			
			sensor.timestamp 	= strtol( buffer, &pCharIdx, 10 );
			sensor.value 		= strtof( pCharIdx, NULL);	

			store_new_measurement( sensor );
		
			//std::cout << "-------new:" << gCurrentStatusIdx << ":" << gSensorHistory[gCurrentStatusIdx -1].value << std::endl;				
	
			// for (int i = 0; i < GRAPH_BARS_NUM; i++)
			// {
			// 	std::cout << "id:" << i << ":" << gSensorHistory[i].value << std::endl;				
			// }

			//std::cout << "Conversion: " << sensor.timestamp << " " << sensor.value << std::endl;
			
		} 
		else if(strcmp( message->topic, SENSOR_STATE_PATT) == 0 )
		{
			// message with a new sensor status

			if( strcmp( (const char*)message->payload, SENSOR_STATUS_SLEEP ) == 0)
			{
				std::cout << "The sensor is sleeping" << std::endl;
				wakeup_sensor( mosq );	
				sensor.isAwaken = false;
			}
			else
			{
				sensor.isAwaken = true;
			}
		}

		//Data is binary, but we cast it to a string because we're crazy.
		//std::cout << message->topic << ":" << (const char*)message->payload << std::endl;
	}	
}

void log_result(const char * subject, int resultCode)
{
	std::cout << subject;

	switch( resultCode )
	{
		case MOSQ_ERR_SUCCESS:
			std::cout << "OK" << std::endl;
			break;
		case MOSQ_ERR_INVAL:
			std::cout << "Invalid parameter" << std::endl;
			break;
		default:
			std::cout << "Error:" << resultCode << std::endl;
			break;
	}
}

void connect_callback(struct mosquitto *mosq, void *userdata, int result)
{
	log_result( "Connected to broker: ", result );
	
	if(result == MOSQ_ERR_SUCCESS)
	{
		//Subscribe to broker information topics on successful connect.
	
		int res = mosquitto_subscribe(mosq, NULL, SENSOR_STATE_PATT, 2);
		log_result( "Connected to sensor's state: ", res );

		res = mosquitto_subscribe(mosq, NULL, SENSOR_VALUE_PATT, 2);
		log_result( "Connected to sensor's measurement: ", res );
	}
}

void wakeup_sensor(struct mosquitto *mosq)
{
	int payloadlen  = 1;
	int qos 		= 2;
	bool retain		= true;

	int res = mosquitto_publish(mosq, NULL,	SENSOR_WAKEUP_PATT,	payloadlen, "1", qos, retain );
	log_result( "Waking up sensor...", res );
}

void publish_callback(struct mosquitto *mosq, void *userdata, int result)
{
	log_result( "Wakeup sent to sensor: ", result );
}

//Draws a simple bar chart with a blue background in the center of the screen.
void draw(SDL_Renderer* renderer, float delta_time)
{
	unsigned char cRed 	 = 0x00;
	unsigned char cGreen = 0x9f; 
	unsigned char cBlue	 = 0xe3;
	unsigned char cAlpha = 0xFF;

	static float time = 0;
	time += delta_time;

	SDL_SetRenderDrawColor(renderer, cRed, cGreen, cBlue , cAlpha );
	SDL_Rect rect = {100, 240, 600, 200};

	//Background
	SDL_RenderFillRect(renderer, &rect);

	//Bars
	int padding = 2;
	int start_x = 120;
	int y 		= 340;
	int width = (rect.w - (start_x - rect.x) * 2 - padding * (GRAPH_BARS_NUM - 1))/GRAPH_BARS_NUM;

	for (int i = 0; i < GRAPH_BARS_NUM; i++)
	{
		// only draw if there is information available
		if ( i  >= get_history_measurement_size() ) 
			break;			

		//float offset = ((float)sin(time * 4 + i) + 1.0f)/2.f;
		SensorStatus sensor = get_measurement(i);
		
		// set a color for positive and negative values
		if( sensor.value < 0 )
		{
			cRed   = 0xFF;
			cGreen = 0X00;
			cBlue  = 0X00;
		}
		else
		{
			cRed   = 0x00;
			cGreen = 0xFF;
			cBlue  = 0X00;
		}

		float offset = sensor.value;
		int height = offset * 40;

		// start drawing from the right to left
		start_x = 120 + (GRAPH_BARS_NUM -1 - i)*(padding + width);

		SDL_Rect bar = {start_x, y - height, width, height};

		// fade out the older measurements
		cAlpha = (unsigned char)((255 / GRAPH_BARS_NUM) * (GRAPH_BARS_NUM - i) );
		SDL_SetRenderDrawColor(renderer, cRed, cGreen, cBlue, cAlpha);
		SDL_RenderFillRect(renderer, &bar);
		//std::cout << "sqr:" << i << ":" << sensor.value << std::endl;
	}
	//std::cout << "-----------" << std::endl;
}

int main(int, char**)
{
	SDL_Window 	 *window 	= 0;
	SDL_Renderer *renderer 	= 0;
	struct mosquitto* mosq;
	int code = 0;
	bool run = true;
	unsigned int t0 = 0;
	unsigned int t1 = 0;
	
	mosquitto_lib_init();

	if((mosq = mosquitto_new(0, true, 0)) == 0)
	{
		std::cout << "Failed to initialize mosquitto." << std::endl;
		code = 1;
		goto end;
	}

	mosquitto_connect_callback_set(mosq, connect_callback);
	mosquitto_message_callback_set(mosq, message_callback);
	mosquitto_publish_callback_set(mosq, publish_callback);

	//Init SDL
	if (SDL_Init(SDL_INIT_VIDEO) != 0){
		std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
		code = 1;
		goto end;
	} 

	//Create a window and renderer attached to it.
	window = SDL_CreateWindow("SDL Skeleton", 100, 100, 800, 600, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
	renderer = SDL_CreateRenderer(window, 0, SDL_RENDERER_PRESENTVSYNC);


	if(!window || !renderer)
	{
		std::cout << "SDL_CreateWindow or SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
		code = 1;
		goto end;
	}

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	//Attempt mosquitto connection to local host.
	mosquitto_connect_async(mosq, "amee.interaktionsbyran.se", 1883, 60);

	//Start the mosquitto network thread.
	if(mosquitto_loop_start(mosq) != MOSQ_ERR_SUCCESS) {
		std::cout << "Failed mosquitto init " << mosq << std::endl;
		code = 1;
		goto end;
	}

	//Block untill the user closes the window
	SDL_Event e;

	while (run) 
	{
		t1 = SDL_GetTicks();
		float delta_time = (float)(t1 - t0)/1000.f;

		while( SDL_PollEvent( &e ) != 0)
		{
			if( e.type == SDL_QUIT )
			{
				run = false;
				break;
			}
		}

		if( is_new_measurement_stored() )
		{
			//Clear buffer.
			SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
			SDL_RenderClear(renderer);

			draw(renderer, delta_time);

			SDL_RenderPresent(renderer);
		}

		t0 = t1;
	}


end:
	SDL_Quit();

	//Cleanup mqtt
	mosquitto_loop_stop(mosq, true);
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();

	return code;
}
