/// std
#include <iostream>
#include <string>

/// boost
#include "boost/program_options.hpp"

/// FairRoot/FairMQ
#include "FairMQLogger.h"
#include "FairMQParser.h"
#include "FairMQProgOptions.h"

//////////////////////////////////////////////////////////////
/// main
//////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{    
    
    try
    {
        FairMQProgOptions config;
    
        po::options_description format_desc("XML input");
        format_desc.add_options() 
            ("xml.config.node.root",  po::value<std::string>()->default_value("fairMQOptions"), "xml root node ")
            ;
       
        po::options_description io_file_opt_desc("I/O file Options");
        io_file_opt_desc.add_options() 
            ("input.file.name",     po::value<std::string>(), "input file name")
            ("input.file.tree",     po::value<std::string>(), "input tree name")
            ("input.file.branch",   po::value<std::string>(), "input branch name")
            ("output.file.name",    po::value<std::string>(), "output file name")
            ("output.file.tree",    po::value<std::string>(), "output tree name")
            ("output.file.branch",  po::value<std::string>(), "output branch name")
            ;

        config.AddToCmdLineOptions(format_desc,true);
        config.AddToCmdLineOptions(io_file_opt_desc,true);
        
        
        config.EnableCfgFile();// UseConfigFile (by default config file is not defined)
        config.AddToCfgFileOptions(format_desc,false);//false because already added to visible
        config.AddToCfgFileOptions(io_file_opt_desc,false);
        
        // Parse command line and config file
        if(config.ParseAll(argc,argv))
            return 0;
        
        // Set severity level (Default is 0=DEBUG)
        int verbosity=config.GetValue<int>("verbosity");
        FairMQLogger::Level lvl=static_cast<FairMQLogger::Level>(verbosity);
        SET_LOGGER_LEVEL(lvl);
        
        // parse XML file
        std::string filename;
        std::string XMLrootNode;

        filename=config.GetValue<std::string>("config-xml-file");
        XMLrootNode=config.GetValue<std::string>("xml.config.node.root");
        std::string id=config.GetValue<std::string>("id");
        config.UserParser<FairMQParser::XML>(filename,id,XMLrootNode);
        
        
    }
    catch (std::exception& e)
    {
        MQLOG(ERROR) << e.what();
        return 1;
    }
    return 0;
}






