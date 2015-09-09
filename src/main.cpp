#include <iostream>
#include <SDL2/SDL.h>
#include <mosquitto.h>
#include <math.h>

// Sensor Patterns
const char* SENSOR_STATE_PATT 	= "recruitment/ciot/state";
const char* SENSOR_VALUE_PATT 	= "recruitment/ciot/sensor1";
const char* SENSOR_WAKEUP_PATT 	= "recruitment/ciot/wake";

// sensor status response
const char* SENSOR_STATUS_SLEEP = "sleep";
const char* SENSOR_STATUS_AWAKE = "awake";

void message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message);
void log_result(const char * subject, int resultCode);
void connect_callback(struct mosquitto *mosq, void *userdata, int result);
void draw(SDL_Renderer* renderer, float delta_time);
int  main(int, char**);
void wakeup_sensor(struct mosquitto *mosq);

void message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
	//Simply print the data to standard out.
	if(message->payloadlen)
	{
		if( strcmp( message->topic, SENSOR_VALUE_PATT) == 0 )
		{

		}
		else if(strcmp( message->topic, SENSOR_STATE_PATT) == 0 )
		{
			if( strcmp( (const char*)message->payload, SENSOR_STATUS_SLEEP ) == 0)
			{
				std::cout << "The sensor is sleeping" << std::endl;
				wakeup_sensor( mosq );	
			}
		}

		//Data is binary, but we cast it to a string because we're crazy.
		std::cout << message->topic << ": " << (const char*)message->payload << std::endl;
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
		log_result( "Connected to sensor state: ", res );

		res = mosquitto_subscribe(mosq, NULL, SENSOR_VALUE_PATT, 2);
		log_result( "Connected to sensor value: ", res );

	}
}

void wakeup_sensor(struct mosquitto *mosq)
{
	int payloadlen  = 1;
	int qos 		= 2;
	bool retain		= true; // yes, retain the msg on the broker

	int res = mosquitto_publish(mosq, 
								NULL, 
								SENSOR_WAKEUP_PATT, 
								payloadlen, 
								"1",
								qos, 
								retain 
							  	);
	log_result( "Waking up sensor...", res );
}

void publish_callback(struct mosquitto *mosq, void *userdata, int result)
{
	log_result( "Wakeup sent to sensor: ", result );
}

//Draws a simple bar chart with a blue background in the center of the screen.
void draw(SDL_Renderer* renderer, float delta_time)
{
	static float time = 0;
	time += delta_time;

	SDL_SetRenderDrawColor(renderer, 0x00, 0x9f, 0xe3, 0xFF );
	SDL_Rect rect = {100, 240, 600, 120};

	//Background
	SDL_RenderFillRect(renderer, &rect);


	//Bars
	int barsCount = 6;
	int padding = 2;
	int start_x = 120;
	int y = 340;
	int width = (rect.w - (start_x - rect.x) * 2 - padding * (barsCount - 1))/barsCount;

	//Draw the bars with a simple sine-wave modulation on height and transparency.
	for (int i = 0; i < barsCount; ++i)
	{
		float offset = ((float)sin(time * 4 + i) + 1.0f)/2.f;
		int height = offset * 20 + 20;
		SDL_Rect bar = {start_x, y - height, width, height};
		SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, (unsigned char)(255 * (offset/2 + 0.5f)));
		SDL_RenderFillRect(renderer, &bar);
		start_x += padding + width;
	}
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

		//Clear buffer.
		SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
		SDL_RenderClear(renderer);

		draw(renderer, delta_time);

		SDL_RenderPresent(renderer);

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
