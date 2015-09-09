#include <iostream>
#include <SDL2/SDL.h>
#include <mosquitto.h>
#include <math.h>

void message_callback(struct mosquitto *mosq, void *userdata, const struct mosquitto_message *message)
{
	//Simply print the data to standard out.
	if(message->payloadlen)
	{
		//Data is binary, but we cast it to a string because we're crazy.
		std::cout << message->topic << ": " << (const char*)message->payload << std::endl;
	}
}

void connect_callback_log(int resultCode)
{
	switch( resultCode )
	{
		case MOSQ_ERR_SUCCESS:
			std::cout << "OK" << std::endl;
			break;
		case MOSQ_ERR_INVAL:
			std::cout << "Invalid parameter" << std::endl;
			break;
		default:
			break;
	}
}

void connect_callback(struct mosquitto *mosq, void *userdata, int result)
{
	if(result == MOSQ_ERR_SUCCESS)
	{
		std::cout << "Connected to broker." << std::endl;
		//Subscribe to broker information topics on successful connect.
		mosquitto_subscribe(mosq, NULL, "$SYS/#", 2);
		
		int res = mosquitto_subscribe(mosq, NULL, "$SYS/recruitment/ciot/state", 2);
		std::cout << "Connected to sensor state: ";
		connect_callback_log( res );

		res = mosquitto_subscribe(mosq, NULL, "recruitment/ciot/sensor1", 2);
		std::cout << "Connected to sensor value: ";
		connect_callback_log( res );

	}
	else
	{
		std::cout << "Failed to connect to broker :(" << std::endl;
	}
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
	int count = 6;
	int padding = 2;
	int start_x = 120;
	int y = 340;
	int width = (rect.w - (start_x - rect.x) * 2 - padding * (count - 1))/count;

	//Draw the bars with a simple sine-wave modulation on height and transparency.
	for (int i = 0; i < count; ++i)
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
	SDL_Window *window = 0;
	SDL_Renderer *renderer = 0;
	int code = 0;
	struct mosquitto* mosq;
	bool run = true;
	unsigned int t0 = 0, t1 = 0;
	
	mosquitto_lib_init();

	if((mosq = mosquitto_new(0, true, 0)) == 0)
	{
		std::cout << "Failed to initialize mosquitto." << std::endl;
		code = 1;
		goto end;
	}

	mosquitto_connect_callback_set(mosq, connect_callback);
	mosquitto_message_callback_set(mosq, message_callback);

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

		// check if the devices is sleeping at recruitment/ciot/state

		// wake up the device at recruitment/ciot/wake = 1

		// ask for the data at recruitment/ciot/sensor1

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
