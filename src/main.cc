#include "app.hh"

#include <fcntl.h>
#include <locale>

int
main(int argc, char* argv[])
{
    std::locale::global(std::locale(""));

#ifdef NDEBUG
    // close(STDERR_FILENO); /* hide libmpg123 errors */
#endif

    bool newArgv = false;
    std::vector<std::string> input;
    if (argc < 2) /* use stdin instead */
    {
        newArgv = true;

        /* force pipe */
        int flags = fcntl(0, F_GETFL, 0);
        fcntl(0, F_SETFL, flags | O_NONBLOCK);

        std::string s;
        while (std::getline(std::cin, s))
            input.push_back(std::move(s));

        /* make fake `argv` so core code works as usual */
        argc = input.size() + 1;
        argv = new char*[argc];

        for (int i = 1; i < argc; i++)
            argv[i] = input[i - 1].data();
    }

    app::PipeWirePlayer p(argc, argv);
    p.playAll();

    if (newArgv) delete[] argv;
}
