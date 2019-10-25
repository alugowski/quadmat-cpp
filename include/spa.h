// Copyright (C) 2019 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_SPA_H
#define QUADMAT_SPA_H

#include <map>

#include "config.h"

namespace quadmat {

    /**
     * A sparse SpA is essentially a map.
     */
    template <typename T, typename IT, typename ADDER = std::plus<T>, typename CONFIG=basic_settings>
    class sparse_spa {
    public:
        /**
         * @param size ignored here. Used in dense SpA.
         */
        explicit sparse_spa(size_t size, const ADDER& adder = ADDER()) : adder(adder) {}

        void update(const IT key, const T& value) {
            auto it = m.find(key);
            if (it == m.end()) {
                // insert
                m.emplace_hint(it, key, value);
            } else {
                // replace
                it->second = adder(it->second, value);
            }
        }

        auto begin() const {
            return m.begin();
        }

        auto end() const {
            return m.end();
        }

        void clear() {
            m.clear();
        }

    protected:
        const ADDER& adder;
        std::map<IT, T, std::less<IT>, typename CONFIG::template TEMP_ALLOC<std::pair<const IT, T>>> m;
    };
}

#endif //QUADMAT_SPA_H
