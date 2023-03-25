// emuDeeS.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <filesystem>
#include <iomanip>
#include <sstream>

#define SDL_MAIN_HANDLED
#include <SDL3/SDL.h>

#include "core.h"
#include "nds_icon.h"


using i16 = int16_t;
using u32 = uint32_t;


Core* core = nullptr;

SDL_Gamepad* gamepad = nullptr;
SDL_Window* window = nullptr;
SDL_Renderer* renderer = nullptr;

bool running = true;

int getTouchX(int x, int y) { return x; }
int getTouchY(int x, int y) { return y - 192; }

void HandleMouseButtonDown(SDL_Event event) {
	float x, y;
	SDL_GetMouseState(&x, &y);

	core->input.pressScreen();
	core->spi.setTouch(getTouchX(x, y), getTouchY(x, y));
}

void HandleMouseButtonUp() {
	core->input.releaseScreen();
	core->spi.clearTouch();
}

void HandleKey(SDL_Event event) {
	switch (event.key.keysym.scancode) {
	case SDL_SCANCODE_W:
		event.key.type == SDL_EVENT_KEY_DOWN ? core->input.pressKey(6) : core->input.releaseKey(6);
		break;
	case SDL_SCANCODE_A:
		event.key.type == SDL_EVENT_KEY_DOWN ? core->input.pressKey(5) : core->input.releaseKey(5);
		break;
	case SDL_SCANCODE_S:
		event.key.type == SDL_EVENT_KEY_DOWN ? core->input.pressKey(7) : core->input.releaseKey(7);
		break;
	case SDL_SCANCODE_D:
		event.key.type == SDL_EVENT_KEY_DOWN ? core->input.pressKey(4) : core->input.releaseKey(4);
		break;
	case SDL_SCANCODE_RETURN:
	case SDL_SCANCODE_RETURN2:
		event.key.type == SDL_EVENT_KEY_DOWN ? core->input.pressKey(0) : core->input.releaseKey(0);
		break;
	case SDL_SCANCODE_ESCAPE:
		event.key.type == SDL_EVENT_KEY_DOWN ? core->input.pressKey(1) : core->input.releaseKey(1);
		break;
	default:
		break;
	}
}

void HandleController(SDL_Event event) {
	switch (event.gbutton.button) {
	case SDL_GAMEPAD_BUTTON_DPAD_UP:
		event.gbutton.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN ? core->input.pressKey(6) : core->input.releaseKey(6);
		break;
	case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
		event.gbutton.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN ? core->input.pressKey(5) : core->input.releaseKey(5);
		break;
	case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
		event.gbutton.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN ? core->input.pressKey(7) : core->input.releaseKey(7);
		break;
	case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
		event.gbutton.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN ? core->input.pressKey(4) : core->input.releaseKey(4);
		break;

	case SDL_GAMEPAD_BUTTON_A:
		event.gbutton.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN ? core->input.pressKey(0) : core->input.releaseKey(0);
		break;
	case SDL_GAMEPAD_BUTTON_B:
		event.gbutton.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN ? core->input.pressKey(1) : core->input.releaseKey(1);
		break;
	case SDL_GAMEPAD_BUTTON_X:
		event.gbutton.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN ? core->input.pressKey(10) : core->input.releaseKey(10);
		break;
	case SDL_GAMEPAD_BUTTON_Y:
		event.gbutton.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN ? core->input.pressKey(11) : core->input.releaseKey(11);
		break;

	case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
		event.gbutton.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN ? core->input.pressKey(9) : core->input.releaseKey(9);
		break;
	case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
		event.gbutton.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN ? core->input.pressKey(8) : core->input.releaseKey(8);
		break;
	}
}

void RenderVideo(uint32_t* framebuffer) {
	SDL_Texture* texture1 = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_BGR888, SDL_TEXTUREACCESS_STATIC, 256, 192 * 2);
	SDL_UpdateTexture(texture1, nullptr, framebuffer, 256 * sizeof(u32));
	SDL_RenderTexture(renderer, texture1, nullptr, nullptr);
}

std::string fps_from_start(Uint64 start) {
	Uint64 end = SDL_GetPerformanceCounter();
	float elapsed = (end - start) / (float)SDL_GetPerformanceFrequency();

	std::stringstream stream;
	stream << std::fixed << std::setprecision(0) << (1.0f / elapsed);
	std::string s = stream.str();
	return s;
}

i16 audioBuffer[1024 * 2];
void AudioCallback(void* userdata, Uint8* stream, int len) {
	u32* original = core->spu.getSamples(699);
	for (int i = 0; i < 1024; i++) {
		u32 sample = original[i * 699 / 1024];
		audioBuffer[i * 2 + 0] = sample >> 0;
		audioBuffer[i * 2 + 1] = sample >> 16;
	}

	SDL_memcpy(stream, audioBuffer, len);
}


SDL_AudioDeviceID device;
SDL_AudioSpec want, have;
int main(int argc, char* argv[]) {
	SDL_Init(SDL_INIT_AUDIO | SDL_INIT_GAMEPAD | SDL_INIT_VIDEO);
	SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "directsound");


	SDL_Surface* icon_surface = nullptr;
	if (argc == 2) {
		core = new Core(argv[1]);
		icon_surface = SDL_CreateSurfaceFrom(CARTRIDGE(argv[1]).GetIcon(), 32, 32, 32 * sizeof(uint32_t), SDL_PIXELFORMAT_BGR888);
	} 
	else if (std::filesystem::exists("files/rom.nds")) {
		core = new Core("files/rom.nds");
		icon_surface = SDL_CreateSurfaceFrom(CARTRIDGE("files/rom.nds").GetIcon(), 32, 32, 32 * 4, SDL_PIXELFORMAT_BGR888);
	}
	else {
		core = new Core("", "");
	}

	int gamepads = 0;
	SDL_GetGamepads(&gamepads);
	if (gamepads > 0)
		gamepad = SDL_OpenGamepad(gamepads);
	printf(SDL_GetError());


	window = SDL_CreateWindow("emuDeeS", 256, 192 * 2, 0);
	SDL_SetWindowIcon(window, icon_surface);
	renderer = SDL_CreateRenderer(window, nullptr, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

	SDL_zero(want);
	want.freq = 48000;
	want.format = AUDIO_S16;
	want.channels = 2;
	want.samples = 1024;
	want.callback = AudioCallback;
	device = SDL_OpenAudioDevice(nullptr, SDL_FALSE, &want, &have, 0);
	SDL_PlayAudioDevice(device);


	SDL_Event event;
	bool held = false;
	while (running) {
		Uint64 start = SDL_GetPerformanceCounter();
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
			case SDL_EVENT_GAMEPAD_BUTTON_UP:
				HandleController(event);
				break;
			case SDL_EVENT_KEY_DOWN:
			case SDL_EVENT_KEY_UP:
				HandleKey(event);
				break;
			case SDL_EVENT_MOUSE_BUTTON_DOWN:
				held = true;
				HandleMouseButtonDown(event);
				break;
			case SDL_EVENT_MOUSE_BUTTON_UP:
				held = false;
				HandleMouseButtonUp();
				break;
			case SDL_EVENT_MOUSE_MOTION:
				float x, y;
				if (held && SDL_GetMouseState(&x, &y) & SDL_BUTTON_LMASK)
					core->spi.setTouch(getTouchX(x, y), getTouchY(x, y));

				break;
			case SDL_EVENT_QUIT:
				exit(0);
				break;
			}
		}
		
		core->runFrame();
		u32 framebuffer[256 * 192 * 2];
		core->gpu.getFrame(framebuffer, false);

		SDL_RenderClear(renderer);
		RenderVideo(framebuffer);
		SDL_RenderPresent(renderer);

		Uint64 end = SDL_GetPerformanceCounter();
		float elapsed = (end - start) / (float)SDL_GetPerformanceFrequency();
		SDL_SetWindowTitle(window, std::format("emuDeeS ({})", (int)(1.0f / elapsed)).c_str());
	}

	SDL_Quit();

	return 0;
}