// Copyright (C) 2019-2020 Adam Lugowski
// All Rights Reserved.

#ifndef QUADMAT_INCLUDE_QUADMAT_QUADTREE_PARALLEL_TREE_DESTRUCTOR_H_
#define QUADMAT_INCLUDE_QUADMAT_QUADTREE_PARALLEL_TREE_DESTRUCTOR_H_

#include <queue>

#include "tbb/task_group.h"

#include "quadmat/quadtree/inner_block.h"

namespace quadmat {

    /**
     * Fast destruction of a quadtree.
     *
     * Destruction of a quadtree happens automatically via node destructors. This process is sequential and can
     * take non-trivial time for large trees. This class parallelizes the process.
     *
     * WARNING: This destruction process modifies the tree.
     * Only use this process if nothing in the tree will be used again.
     */
    template <typename T, typename Config>
    class ParallelTreeDestructor {
    public:
        using InnerType = std::shared_ptr<InnerBlock<T, Config>>;

        /**
         * Destroy the quadtree in parallel, possibly modifying it in the process.
         *
         * @param bc root of tree to destroy
         * @param p parallelism
         */
        static void Destroy(const std::shared_ptr<BlockContainer<T, Config>>& bc, int p) {
            if (bc.get() == nullptr) {
                return;
            }

            // Find p root nodes
            auto roots = std::vector<Destructee, typename Config::template TempAllocator<Destructee>>();

            for (int pos = 0; pos < bc->GetNumChildren(); ++pos) {
                roots.emplace_back(Destructee{
                    .bc = bc.get(),
                    .pos = pos
                });
            }

            while (roots.size() > 0 && roots.size() < p) {
                auto destructee = roots.front();
                roots.erase(roots.begin());

                auto node = destructee.bc->GetChild(destructee.pos);

                if (!std::holds_alternative<InnerType>(node)) {
                    continue;
                }

                auto inner = std::get<InnerType>(node);

                for (int pos = 0; pos < inner->GetNumChildren(); ++pos) {
                    roots.emplace_back(Destructee{
                        .bc = inner.get(),
                        .pos = pos
                    });
                }
            }

            // Destroy each root in a separate thread.
            // Smart pointer destructors will propagate destruction of all child nodes of the roots.
            tbb::task_group g;
            for (auto destructee : roots) {
                g.run([destructee] {
                    destructee.bc->SetChild(destructee.pos, std::monostate());
                });
            }
            g.wait();
        }

    protected:
        struct Destructee {
            BlockContainer<T, Config>* bc;
            int pos;
        };
    };
}

#endif //QUADMAT_INCLUDE_QUADMAT_QUADTREE_PARALLEL_TREE_DESTRUCTOR_H_
