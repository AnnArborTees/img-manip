#include <string>
#include <iostream>
#include "routine.h"
#include <stdio.h>
#define _USE_MATH_DEFINES
#include <math.h>
#include <memory>
#include <functional>
#include <unordered_map>

using namespace mockbot;

bool run(Session &session, int* pargc, char*** pargv) {
    if ((*pargc) < 1)
        return false;

    std::string subcommand((*pargv)[0]);
    int return_code;
    int args_needed;

    if (Routine::subcommands.count(subcommand) > 0) {
        auto routine = Routine::subcommands[subcommand];
        args_needed = routine->args_used();

        //
        // Make sure we have enough args
        //
        if (args_needed > *pargc) {
            std::cerr << "Not enough arguments for \"" << subcommand << "\" "
                      << "(needs " << args_needed << ", have " << *pargc << " left)\n";
            return false;
        }

        //
        // Perform the routine
        //
        return_code = routine->perform(session, (*pargv));
    }
    else {
        std::cerr << "Invalid subcommand \"" << subcommand << "\"\n";
        return false;
    }

    if (return_code != 0) {
        std::cerr << "Subcommand \"" << subcommand << "\" returned " << return_code << '\n';
        return false;
    }

    //
    // "Consume" the arguments used
    //
    *pargc -= args_needed;
    *pargv += args_needed;

    return args_needed != 0;
}

int examine(int argc, char** argv) {
  enum {
    ARGUMENT,
    COMMAND
  } state = COMMAND;
  int argnum = 0;

  // Remove "examine" from args
  argc -= 1;
  argv += 1;

  for (int i = 0; i < argc; ++i) {
    std::string arg = argv[i];

    if (Routine::subcommands.count(arg) > 0) {
      state = COMMAND;
      argnum = 1;
      std::cout << '\n';
    }
    else
      state = ARGUMENT;

    switch (state) {
    case ARGUMENT:
      std::cout << '(' << argnum << ')' << '\t';
      ++argnum;
    case COMMAND:
      std::cout << arg << '\n';
    }
  }

  return 0;
}

int main(int argc, char** argv) {
    // First arg is application path
    argc -= 1;
    argv += 1;

    const std::string examine_cmd = "examine";

    if (argc >= 1 && argv[0] == examine_cmd)
        return examine(argc, argv);
    else if (argc == 0) {
        print_all_help_texts();
        return 0;
    }

    Session session;
    while (run(session, &argc, &argv)) {}

    return 0;
}
