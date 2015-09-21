/* 
 * File:   fairroot_null_deleter.h
 * Author: winckler
 *
 * Created on September 21, 2015, 11:50 AM
 */

#ifndef FAIRROOT_NULL_DELETER_H
#define	FAIRROOT_NULL_DELETER_H
// boost like null_deleter introduced for backward compability if boost version < 1.56

namespace fairroot
{
    struct null_deleter
    {
        //! Function object result type
        typedef void result_type;
        /*!
         * Does nothing
         */
        template< typename T >
        void operator() (T*) const noexcept {}
    };

}
#endif	/* FAIRROOT_NULL_DELETER_H */

