#pragma once

// Launch embedded backend (lab5 server logic) and listen on localhost:8080.
// simulate=true -> generate samples via Simulator.
void start_backend(bool simulate = true);
// Stop backend and join worker thread.
void stop_backend();
