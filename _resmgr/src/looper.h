#pragma once
#include "pch.h"

struct Looper : xx::Engine<Looper>, xx::GDesign<1280, 800, 60> {
	xx::Task<> DrawTask();
};

extern Looper gLooper;
