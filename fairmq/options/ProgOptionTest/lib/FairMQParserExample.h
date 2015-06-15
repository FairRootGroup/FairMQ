/* 
 * File:   FairMQParserExample.h
 * Author: winckler
 *
 * Created on May 14, 2015, 5:01 PM
 */

#ifndef FAIRMQPARSEREXAMPLE_H
#define	FAIRMQPARSEREXAMPLE_H

// FairRoot
#include "FairMQChannel.h"
#include "FairMQParser.h"

// Boost
#include <boost/property_tree/ptree.hpp>

// std
#include <string>
#include <vector>
#include <map>


namespace FairMQParser
{
    
    ////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////// XML ////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////
    
    // xml example 1
    ////////////////////////////////////////////////////////////////////////////
    struct XML 
    {
        FairMQMap UserParser(const std::string& filename, const std::string& device_id, const std::string& root_node="fairMQOptions");
        FairMQMap UserParser(std::stringstream& input_ss, const std::string& device_id, const std::string& root_node="fairMQOptions");
    };
    
    // xml example 2
    ////////////////////////////////////////////////////////////////////////////
    struct MQXML2 
    {
        boost::property_tree::ptree UserParser(const std::string& filename);
    };

    // xml example 3
    ////////////////////////////////////////////////////////////////////////////
    struct MQXML3
    {
        boost::property_tree::ptree UserParser(const std::string& filename, const std::string& root_node);
    };
    
    
    
    
    ////////////////////////////////////////////////////////////////////////////
    // template function iterating over the whole boost property tree
    template <typename Input_tree_It, typename Output_tree_It, typename Compare_key>
    void ProcessTree(Input_tree_It first, Input_tree_It last, Output_tree_It dest, Compare_key compare)
    {
        //typedef typename std::iterator_traits<Input_tree_It>::reference reference;

        if (first == last)
        {
            return;
        }

        auto begin = first->second.begin ();
        auto end = first->second.end ();

        if (begin != end)
        {
            ProcessTree (begin, end, dest, compare);
        }

        if (compare (first->first))
        {
            dest = *first;
        }

        ProcessTree (++first, last, dest, compare);
    }
    
    class no_id_exception: public std::exception
    {
        virtual const char* what() const throw()
        {
          return "Empty string for the device-id in FairMQParser::ptreeToMQMap(...) function";
        }
    };
    
} //  end FairMQParser namespace
#endif	/* FAIRMQPARSEREXAMPLE_H */

