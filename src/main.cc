#include "app.hh"

#include <locale>

int
main(int argc, char* argv[])
{
    std::locale::global(std::locale(""));

    app::PipeWirePlayer p(argc, argv);
    p.playAll();
}
