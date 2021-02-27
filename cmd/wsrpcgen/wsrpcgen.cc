
#include <cassert>
#include <fstream>
#include <getopt.h>

#include "wsrpcgen.hh"

void OutStream::writeTo(std::string fileName)
{
    std::ofstream out(fileName);
    out << txt;
    out.close();
    txt.clear();
}

int main(int argc, char *argv[])
{
    OutStream os;
    std::list<std::string> includeDirs;
    bool svc = false, clnt = false, head = false, ser = false;
    char c;
    std::string outName = "rpc";

    while ((c = getopt(argc, argv, "hscxo:I:")) != -1)
        switch (c)
        {
        case 's':
            svc = true;
            break;
        case 'c':
            clnt = true;
            break;
        case 'h':
            head = true;
            break;
        case 'x':
            ser = true;
            break;
        case 'o':
            outName = optarg;
            break;
        case 'I':
            includeDirs.push_back(optarg);
            break;
        case '?':
            if (optopt == 'o')
                fprintf(stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint(optopt))
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
            return 1;
        default:
            abort();
        }

    assert((argc - optind) == 1);

    auto res = MVST_Parser::parseFile(argv[optind], includeDirs);
    res->synthInScope(NULL);

    size_t lSlashPos = outName.find_last_of('/');
    std::string shortOutName;
    if (lSlashPos == std::string::npos)
        shortOutName = outName;
    else
        shortOutName = outName.substr(lSlashPos + 1);

    if (head)
    {
        res->genHeader(os);
        os.writeTo(outName + ".hh");
    }
    if (ser)
    {
        os.add("#include <cassert>\n");
        os.add("#include \"" + shortOutName + ".hh\"\n");
        os.add(res->code);
        res->genSerialise(os);
        os.writeTo(outName + "_conv.cc");
    }
    if (svc)
    {
        os.add("#include \"" + shortOutName + ".hh\"\n");
        os.add(res->code);
        res->genServer(os);
        os.writeTo(outName + "_svc.cc");
    }
    if (clnt)
    {
        os.add("#include \"" + shortOutName + ".hh\"\n");
        os.add(res->code);
        res->genClient(os);
        os.writeTo(outName + "_clnt.cc");
    }
}