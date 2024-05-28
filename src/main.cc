#include "app.hh"

#include <locale>

int
main(int argc, char* argv[])
{
    std::locale::global(std::locale(""));

#ifdef NDEBUG
    close(STDERR_FILENO); /* hide libmpg123 errors */
#endif

    app::PipeWirePlayer p(argc, argv);
    p.playAll();
}
