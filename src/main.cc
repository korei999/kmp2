#include "app.hh"
#ifdef MPRIS
#include "mpris.hh"
#endif

#include <locale>

int
main(int argc, char* argv[])
{
    std::locale::global(std::locale(""));

#ifdef NDEBUG
    close(STDERR_FILENO); /* hide libmpg123 errors */
#endif

    app::PipeWirePlayer p(argc, argv);

#ifdef MPRIS
    mpris::init(&p);
#endif

    p.playAll();

#ifdef MPRIS
    mpris::clean();
#endif
}
