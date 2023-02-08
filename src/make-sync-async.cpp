#include <Arduino.h>

// The current code has synchronous/blocking loops with delays like this
// This example blocks anything else from happening for 10 seconds 
void BlockingDelayLoopFunction()
{
    for (int i = 0; i < 1000; i++)
    {
        // do something
        delay(10);
    }
}
// Because the loops are blocking, they can't run forever, or you wouldn't be able to send more commands.

// You should make nonblocking versions of the functions, something like this
uint32_t loop_counter{0};
uint32_t start_time; // will be set when we initialize the "loop"

void StartNonblockingLoopFunction()
{
    loop_counter = 0;
    start_time = millis();
}

void ProcessNonblockingLoopFunction()
{
    const uint32_t delay_time{10};
    if (millis() >= start_time + loop_counter * delay_time) // if current time is after when the current loop iteration should be over
    {
        // do something
        loop_counter++; // now on the next loop iteration
    }
}

// the process function can then be called repetitively from the main loop as often as you want
void MainLoop()
{
    // do lots of things
    // if (should start nonblocking function)
    //{
    //     StartNonblockingLoopFunction();
    // }
    ProcessNonblockingLoopFunction();
}

// you should probably have an enum class of modes so you know which process function to call
enum class Mode
{
    SolidColor,
    LarsonScanner,
    EtCetera
};

// keep track of current mode
Mode current_mode;

// in your main loop, use a switch case to call the right process function
void SwitchCaseForMainLoop()
{
    switch (current_mode)
    {
    case Mode::SolidColor:
        // ProcessSolidColor();
        break;
    case Mode::LarsonScanner:
        // ProcessLarsonScanner();
        break;
        // et cetera

    default:
        break;
    }
}

// then also change the main loop to set the current state and do any initialization when you click a button
void MainLoopProcessCommand()
{
    // if got a packet
    String packetbuffer = "incoming packet";
    if (packetbuffer[1] == 'C' && (current_mode != Mode::SolidColor /*|| current_color != incoming_color*/))
    {
        loop_counter = 0;
        start_time = millis();
        current_mode = Mode::SolidColor;
    }
    else if (packetbuffer[1] == 'B')
    {
        // check which button pressed
        // if maps to different mode, set loop counter and start time
    }

    // main loop should then have the switch case
}