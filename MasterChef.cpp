#include <getopt.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string>
#include "StepList.h"

using namespace std;

StepList* recipeSteps;
vector<int>* completedSteps;
int completeCount = 0;

void PrintHelp() // Given
{

	cout << "Usage: ./MasterChef -i <file>\n\n";
	cout<<"--------------------------------------------------------------------------\n";
	cout<<"<file>:    "<<"csv file with Step, Dependencies, Time (m), Description\n";
	cout<<"--------------------------------------------------------------------------\n";
	exit(1);
}

string ProcessArgs(int argc, char** argv) // Given
{
	string result = "";
	// print help if odd number of args are provided
	if (argc < 3) {
		PrintHelp();
	}

	while (true)
	{
		const auto opt = getopt(argc, argv, "i:h");

		if (-1 == opt)
			break;

		switch (opt)
		{
		case 'i':
			result = std::string(optarg);
			break;
		case 'h': // -h or --help
		default:
			PrintHelp();
			break;
		}
	}

	return result;
}

// Creates and starts a timer given a pointer to the step to start and when it will expire in seconds.
void makeTimer( Step *timerID, int expire) // Given
{
    struct sigevent te;
    struct itimerspec its;

    /* Set and enable alarm */
    te.sigev_notify = SIGEV_SIGNAL;
    te.sigev_signo = SIGRTMIN;
    te.sigev_value.sival_ptr = timerID;
    timer_create(CLOCK_REALTIME, &te, &(timerID->t_id));

    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;
    its.it_value.tv_sec = expire;
    its.it_value.tv_nsec = 0;
    timer_settime(timerID->t_id, 0, &its, NULL);
}

// Triggers when the time for the step has elapsed.
// the siginfo_t* pointer holds the step information in the si_value.sival_ptr
// You will need to print out that the step has completed
// Mark the step to be removed as a dependency, and trigger the remove dep handler.
static void timerHandler( int sig, siginfo_t *si, void *uc )
{
	// Retrieve timer pointer from the si->si_value
    Step* comp_item = (Step*)si->si_value.sival_ptr;

	// Officially complete the step using completedSteps and completeCount
	completedSteps->push_back(comp_item->id);
	++completeCount;
	// Print completed step
	comp_item->PrintComplete();
	// Call RemoveDepHandler()
	raise(SIGUSR1);

	// Ready to remove that dependency, call the trigger for the appropriate handler
}

// Removes the copmleted steps from the dependency list of the step list.
// Utilize the completedSteps vector and the RemoveDependency method.
void RemoveDepHandler(int sig) {
	// For each step that has been completed since last run, remove it as a dependency
	for (size_t i = 0; i < completedSteps->size(); ++i) {
		recipeSteps->RemoveDependency(completedSteps->at(i));
	}	
}

// Associate the signals to the signal handlers as appropriate
// Continuously check what steps are ready to be run, and start timers for them with makeTimer()
// run until all steps are done.
int main(int argc, char **argv)
{
	string input_file = ProcessArgs(argc, argv);
	if(input_file.empty()) {
		exit(1);
	}
	
	// Initialize global variables
	completedSteps = new vector<int>();
	recipeSteps = new StepList(input_file);

    /* Set up signal handler. */
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = timerHandler;
    sigemptyset(&sa.sa_mask);

	// Associate the signal SIGRTMIN with the sa struct, for the timerHandler
	// Associate the RemoveDepHandler with the SIGUSR1 signal, for removing dependencies
	sigaction(SIGRTMIN, &sa, NULL);
	signal(SIGUSR1, RemoveDepHandler);
	// While there are still steps to be completed
	while (completeCount < recipeSteps->Count()) {
		// Get the ready steps
		vector<Step*> readySteps = recipeSteps->GetReadySteps();
		// For each ready step
		for (size_t i = 0; i < readySteps.size(); ++i) {
			// Mark it as running and create a timer for it
			Step* step = readySteps.at(i);
			step->running = true;
			makeTimer(step, step->duration);
		}
	}
	// All done, Enjoy!
	cout << "Enjoy!" << endl;
}