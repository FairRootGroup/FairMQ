/// std
#include <iostream>
#include <string>

/// boost
#include "boost/program_options.hpp"

/// FairRoot/FairMQ
#include "FairMQLogger.h"
#include "FairMQParser.h"
#include "FairMQProgOptions.h"

// Parse xml from file
int testXML1(FairMQProgOptions* config)
{
    LOG(info)<<"--------- test XML1 ---------\n";
    std::string filename;
    std::string XMLrootNode;

    filename=config->GetValue<std::string>("config-xml-file");
    XMLrootNode=config->GetValue<std::string>("xml.config.node.root");
    std::string id=config->GetValue<std::string>("id");
    config->UserParser<FairMQParser::XML>(filename,id,XMLrootNode);
    // other xml parser test
    config->UserParser<FairMQParser::MQXML2>(filename);
    config->UserParser<FairMQParser::MQXML3>(filename,"merger");

    LOG(info)<<"--------- test XML1 end ---------\n";
    return 0;
}

// Parse xml from command line
int testXML2(FairMQProgOptions* config)
{
    LOG(info)<<"--------- test XML2 ---------\n";
    std::string XML;
    std::string XMLrootNode;
    std::string id=config->GetValue<std::string>("id");
    XMLrootNode=config->GetValue<std::string>("xml.config.node.root");

    // Note: convert the vector<string> into one string with GetStringValue(key)
    XML=config->GetStringValue("config-xml-string");

    std::stringstream iss;
    iss << XML;
    config->UserParser<FairMQParser::XML>(iss,id,XMLrootNode);

    LOG(info)<<"--------- test XML2 end ---------\n";
    return 0;
}

// Parse json from file
int testJSON1(FairMQProgOptions* config)
{
    LOG(info)<<"--------- test JSON1 ---------\n";
    std::string filename;
    std::string JSONrootNode;
    std::string id=config->GetValue<std::string>("id");

    filename=config->GetValue<std::string>("config-json-file");
    JSONrootNode=config->GetValue<std::string>("json.config.node.root");

    config->UserParser<FairMQParser::JSON>(filename,id,JSONrootNode);

    LOG(info)<<"--------- test JSON1 end ---------\n";
    return 0;
}

// Parse json from command line
int testJSON2(FairMQProgOptions* config)
{
    LOG(info)<<"--------- test JSON2 ---------\n";
    std::string JSON;
    std::string JSONrootNode;
    std::string id=config->GetValue<std::string>("id");
    JSONrootNode=config->GetValue<std::string>("json.config.node.root");

    // Note: convert the vector<string> into one string with GetStringValue(key)
    JSON=config->GetStringValue("config-json-string");

    std::stringstream iss;
    iss << JSON;
    config->UserParser<FairMQParser::JSON>(iss,id,JSONrootNode);

    LOG(info)<<"--------- test JSON2 end ---------\n";
    return 0;
}

int main(int argc, char** argv)
{
    FairMQProgOptions* config= new FairMQProgOptions();
    try
    {
        po::options_description format_desc("XML or JSON input");
        format_desc.add_options()
            ("xml.config.node.root",  po::value<std::string>()->default_value("fairMQOptions"), "xml root node ")
            ("json.config.node.root", po::value<std::string>()->default_value("fairMQOptions"), "json root node ")
        ;

        config->AddToCmdLineOptions(format_desc);

        // Parse command line
        if (config->ParseAll(argc,argv))
        {
            return 0;
        }

        // Set severity level (Default is 0=DEBUG)
        int severity = config->GetValue<int>("severity");
        FairMQLogger::Level lvl=static_cast<FairMQLogger::Level>(severity);
        SET_LOGGER_LEVEL(lvl);

        // Parse xml or json from cmd line or file
        if (config->GetVarMap().count("config-xml-file"))
        {
            testXML1(config);
        }

        if (config->GetVarMap().count("config-xml-string"))
        {
            testXML2(config);
        }

        if (config->GetVarMap().count("config-json-file"))
        {
            testJSON1(config);
        }

        if (config->GetVarMap().count("config-json-string"))
        {
            testJSON2(config);
        }

    }
    catch (std::exception& e)
    {
        LOG(error) << e.what();
        return 1;
    }
    return 0;
}
