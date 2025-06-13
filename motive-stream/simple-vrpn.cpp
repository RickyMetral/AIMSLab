#include <iostream>
#include "vrpn_Tracker.h"

// Callback for position data
void VRPN_CALLBACK handle_tracker(void* userData, const vrpn_TRACKERCB t)
{
    std::cout << "Tracker pos: "
              << t.pos[0] << ", " << t.pos[1] << ", " << t.pos[2] << std::endl;
}

int main()
{
    vrpn_Tracker_Remote* tracker = new vrpn_Tracker_Remote("Tracker0@192.168.1.42"); 

    tracker->register_change_handler(0, handle_tracker);
    while (true) {
        tracker->mainloop();
        vrpn_SleepMsecs(10);  // Avoid hammering CPU
    }

    return 0;
}
