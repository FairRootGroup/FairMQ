/********************************************************************************
 *   Copyright (C) 2025 GSI Helmholtzzentrum fuer Schwerionenforschung GmbH     *
 *                                                                              *
 *              This software is distributed under the terms of the             *
 *              GNU Lesser General Public Licence (LGPL) version 3,             *
 *                  copied verbatim in the file "LICENSE"                       *
 ********************************************************************************/
#include "EventManager.h"

#include <string>
#include <typeindex>

template std::shared_ptr<
    fair::mq::EventManager::Signal<fair::mq::PropertyChangeAsString, std::string>>
    fair::mq::EventManager::GetSignal<fair::mq::PropertyChangeAsString, std::string>(
        const std::pair<std::type_index, std::type_index>& key) const;

template void fair::mq::EventManager::Subscribe<fair::mq::PropertyChangeAsString, std::string>(
    const std::string& subscriber,
    std::function<void(typename fair::mq::PropertyChangeAsString::KeyType, std::string)>);
