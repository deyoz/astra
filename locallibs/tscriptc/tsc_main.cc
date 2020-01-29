#include <iostream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <string>
#include <vector>
#include <cassert>
#include <stdio.h>
#include <string.h>
#include "tsc_ast.h"
#include <unistd.h>

void MacroProcessor(ast::Suite& suite);
void Parse(FILE* in, ast::Suite& suite);
void GenSource(std::ostream& out, ast::Suite& suite, bool color);
void GenVmCode(std::ostream& out, ast::Suite& suite);

struct Options {
    bool showHelp;
    bool nosync;
    bool wrap;
    enum {
        OUT_FMT_SOURCE,
        OUT_FMT_VMCODE
    } outputFormat;
    std::vector<std::string> includeDirs;
    std::vector<std::string> files;

    Options():
        showHelp(false),
        nosync(false),
        wrap(false),
        outputFormat(OUT_FMT_SOURCE)
    {}
};

/* returns shell command */
static std::string Preprocessor(const std::string& fileName, const Options& options)
{
    /* --synclines to generate #line LINENUM "FILENAME" */
    std::string command = "m4 --synclines";
    for (std::vector<std::string>::const_iterator includeDir = options.includeDirs.begin();
         includeDir != options.includeDirs.end();
         ++includeDir)
    {
        command += " --include=\"" + *includeDir + "\"";
    }

    /* if input file name is empty, m4 will read from stdin */
    if (!fileName.empty())
        command += ' ' + fileName;
    return command;
}

static ast::Call* WrapCmd(ast::Seq* cmd)
{
    assert(!cmd->elems.empty());
    ast::Call* call = new ast::Call(
            true,
            cmd->loc(),
            new ast::Seq(new ast::Str(ast::UnknownLoc(), "!!")),
            new ast::Params);
    call->params.push_back(new ast::Param(
                NULL,
                new ast::Seq(new ast::Quote(ast::UnknownLoc(), cmd))));
    call->macroExpandLocations = cmd->elems.front().macroExpandLocations;
    return call;
}

static void Wrap(ast::Seq* seq)
{
    ast::Seq* wrapped = new ast::Seq;
    ast::Seq* cmd = new ast::Seq;
    for (size_t i = 0; i < seq->elems.size(); ++i) {
        ast::Seq::Elem* elem = &seq->elems[i];
        if (elem->call && elem->call->isBlock) {
            if (!cmd->elems.empty()) {
                wrapped->append(WrapCmd(cmd));
                cmd = new ast::Seq;
            }
            wrapped->append(elem->call);
        } else if (elem->str && elem->str->text == "\n") {
            if (!cmd->elems.empty()) {
                wrapped->append(WrapCmd(cmd));
                cmd = new ast::Seq;
            }
        } else
            cmd->elems.push_back(*elem);
    }
    if (!cmd->elems.empty())
        wrapped->append(WrapCmd(cmd));
    seq->elems = wrapped->elems;
}

static void ProcessSuite(ast::Suite& suite, const Options& options)
{
    MacroProcessor(suite);
    if (options.wrap)
        std::for_each(suite.modules.begin(), suite.modules.end(), Wrap);
    switch (options.outputFormat) {
        case Options::OUT_FMT_SOURCE: GenSource(std::cout, suite, isatty(1)); break;
        case Options::OUT_FMT_VMCODE: GenVmCode(std::cout, suite); break;
    }
}

static void ProcessFile(const std::string& fileName, const Options& options)
{
    FILE* pp_pipe = NULL;
    try {
        ast::Suite suite;
        ast::Suite::setCurrentInstance(&suite);
        suite.lexer.nosync = options.nosync;
        suite.lexer.pos.setFile(fileName);

        const std::string pp_cmd = Preprocessor(fileName, options);
        FILE* pp_pipe = popen(pp_cmd.c_str(), "r");
        if (pp_pipe == NULL)
            throw std::runtime_error("failed to open pipe");

        Parse(pp_pipe, suite);

        const int pp_ret = pclose(pp_pipe);
        if (pp_ret != 0) {
            pp_pipe = NULL;
            std::ostringstream err;
            err << pp_cmd << " exited with error status = " << pp_ret;
            throw std::runtime_error(err.str());
        }

        ProcessSuite(suite, options);
        ast::Suite::setCurrentInstance(NULL);
    } catch (...) {
        if (pp_pipe) pclose(pp_pipe);
        throw;
    }
    ast::Suite::setCurrentInstance(NULL);
}

static void Run(const Options& options)
{
    if (options.files.empty())
        ProcessFile("", options); /* input from stdin */
    else
        for (std::vector<std::string>::const_iterator file = options.files.begin();
             file != options.files.end();
             ++file)
        {
            ProcessFile(*file, options);
        }
}

static void Usage(const std::string& exeFile)
{
    std::cout << "usage: " << exeFile <<
        " [OPTIONS] [infile]\n"
        "options:\n"
        "  -h,--help               show this message\n"
        "  -c                      VM code output\n"
        "  -w                      wrap top-level lines in default function call\n"
        "  --include=<directory>   append directory to include path\n"
        "  --nosync                ignore M4 synclines\n";
}

static Options ParseOptions(int argc, char* argv[])
{
    Options options;
    for (int i = 1; i < argc; ++i)
        if (strcmp(argv[i], "--help") == 0)
            options.showHelp = true;
        else if (strstr(argv[i], "--include=") == argv[i])
            options.includeDirs.push_back(argv[i] + strlen("--include="));
        else if (strcmp(argv[i], "--nosync") == 0)
            options.nosync = true;
        else if (*argv[i] == '-') {
            for (const char* c = argv[i] + 1; *c != '\0'; ++c)
                switch (*c) {
                    case 'h': options.showHelp = true; break;
                    case 'c': options.outputFormat = Options::OUT_FMT_VMCODE; break;
                    case 'w': options.wrap = true; break;
                    default: throw std::runtime_error(std::string("unknown option -") + *c);
                }
        } else
            options.files.push_back(argv[i]);
    return options;
}

int main(int argc, char* argv[])
{
    try {
        const Options options = ParseOptions(argc, argv);
        if (options.showHelp)
            Usage(argv[0]);
        else
            Run(options);
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}

