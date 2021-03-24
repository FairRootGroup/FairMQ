/********************************************************************************
* Copyright (C) 2014-2021 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH  *
*                                                                              *
*              This software is distributed under the terms of the             *
*              GNU Lesser General Public Licence (LGPL) version 3,             *
*                  copied verbatim in the file "LICENSE"                       *
********************************************************************************/

#include <fairmq/tools/Unique.h>

#include <boost/program_options.hpp>

#include <iostream>
#include <string>

using namespace std;
using namespace boost::program_options;

int main(int argc, char** argv)
{
    try
    {
        bool hash = false;

        options_description desc("Options");
        desc.add_options()
            ("hash,h", value<bool>(&hash)->implicit_value(true), "Generates UUID and returns its hash.")
            ("help", "Print help");

        variables_map vm;
        store(parse_command_line(argc, argv, desc), vm);

        if (vm.count("help"))
        {
            cout << "UUID generator" << endl << desc << endl;
            return 0;
        }

        notify(vm);

        if (hash)
        {
            std::cout << fair::mq::tools::UuidHash() << std::endl;
        }
        else
        {
            std::cout << fair::mq::tools::Uuid() << std::endl;
        }

        return 0;
    }
    catch (exception& e)
    {
        cerr << "Unhandled Exception reached the top of main: " << e.what() << ", application will now exit" << endl;
        return 2;
    }

    return 0;
}
