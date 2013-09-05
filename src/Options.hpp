
#ifndef OPTIONS
#define OPTIONS

#include <vector>
#include <string>

typedef std::vector<std::string> StringList;

class Options
{
    public:

        static Options& getInstance();
        static bool handleFlags(int argc, char** argv); //should only call once!
        
        bool highVerbosity();
        std::string getConnectionIP();
        std::string getConnectionPort();
        std::string getPassword();

        bool bounceSnapshots();
        bool cycleSnapshots();
        //int renderMode();
        std::string connectionPassword();

    private:
        //Options();
        int handle(const StringList& options, int index);
        bool canGrabNextOption(const StringList& options, int index);
        bool assert(bool condition, const StringList& options, int index);

    private:
        static Options* singleton_;

        bool highVerbosity_, bounceSnapshots_, cycleSnapshots_;
        //int renderMode_;
        std::string connectionIP_;
        std::string connectionPort_;
        std::string authPassword_;
};

#endif

/*
The CLI options should be implemented:
--help [string] Print help screen or help on a particular option and exit.
--license Print license information and quit.
-v or --verbose Increase verbosity level
--version Print application version and exit.

Configuration options in the format "--< option > < value >" or "--< option >=< value >" or "--< option >"
--connect < string=127.0.0.1:36330 > An address and port to connect to in the form :
--bounce-snapshots < boolean=false > When the last snapshot is reached, should the animation run backwards or not.
--cycle-snapshots < boolean=true > Iterate through all available snapshots.
--mode < integer=4 > Render mode.
--password < string= > A password for accessing the remote client.*/
