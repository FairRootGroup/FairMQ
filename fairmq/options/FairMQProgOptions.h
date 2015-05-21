/* 
 * File:   FairMQProgOptions.h
 * Author: winckler
 *
 * Created on March 11, 2015, 10:20 PM
 */

#ifndef FAIRMQPROGOPTIONS_H
#define	FAIRMQPROGOPTIONS_H

#include "FairProgOptions.h"

#include "FairMQChannel.h"
#include <boost/property_tree/ptree.hpp>

namespace pt = boost::property_tree;
class FairMQProgOptions : public FairProgOptions
{
protected:
    typedef std::map<std::string, std::vector<FairMQChannel> > FairMQMap;
    
public:
    FairMQProgOptions();
    virtual ~FairMQProgOptions();
    virtual int ParseAll(const int argc, char** argv, bool AllowUnregistered=false);
    
    
    // external parser, store function 
    template < typename T, typename ...Args>
    int UserParser(Args &&... args)
    {
        try
        {
            Store( T().UserParser(std::forward<Args>(args)...) );
        }
        catch (std::exception& e)
        {
            MQLOG(ERROR) << e.what();
            return 1;
        }
        return 0;
    }
    
    int Store(const po::variables_map& vm)
    {
        fvarmap = vm;
        return 0;
    }
    
    int Store(const pt::ptree& tree)
    {
        fMQtree = tree;
        return 0;
    }
    
    int Store(const FairMQMap& channels)
    {
        fFairMQmap = channels;
        return 0;
    }
    
    
    FairMQMap GetFairMQMap()
    {
        return fFairMQmap;
    }
    
    
protected:
    po::options_description fMQParserOptions;
    pt::ptree fMQtree;
    FairMQMap fFairMQmap;
    
    virtual int NotifySwitchOption();// for custom help & version printing
    void InitOptionDescription();
};


#endif	/* FAIRMQPROGOPTIONS_H */

