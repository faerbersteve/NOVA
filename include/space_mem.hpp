/*
 * Memory Space
 *
 * Copyright (C) 2009-2011 Udo Steinberg <udo@hypervisor.org>
 * Economic rights: Technische Universitaet Dresden (Germany)
 *
 * Copyright (C) 2012 Udo Steinberg, Intel Corporation.
 *
 * This file is part of the NOVA microhypervisor.
 *
 * NOVA is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * NOVA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License version 2 for more details.
 */

#pragma once

#include "config.hpp"
#include "cpu.hpp"
#include "cpuset.hpp"
#include "dpt.hpp"
#include "ept.hpp"
#include "hpt.hpp"
#include "space.hpp"

class Space_mem : public Space
{
    public:
        Hpt loc[NUM_CPU];
        Hpt hpt;
        Dpt dpt;
        union {
            Ept ept;
            Hpt npt;
        };

        mword did;

        Cpuset cpus;
        Cpuset htlb;
        Cpuset gtlb;

        static unsigned did_ctr;

        ALWAYS_INLINE
        inline Space_mem() : did (Atomic::add (did_ctr, 1U)) {}

        ALWAYS_INLINE
        inline size_t lookup (mword virt, Paddr &phys)
        {
            mword attr;
            return hpt.lookup (virt, phys, attr);
        }

        ALWAYS_INLINE
        inline void insert (Quota &quota, mword virt, unsigned o, mword attr, Paddr phys)
        {
            hpt.update (quota, virt, o, phys, attr);
        }

        ALWAYS_INLINE
        inline Paddr replace (Quota &quota, mword v, Paddr p)
        {
            return hpt.replace (quota, v, p);
        }

        INIT
        void insert_root (Quota &quota, uint64, uint64, mword = 0x7);

        bool insert_utcb (Quota &quota, mword, mword = 0);

        bool remove_utcb (mword);

        bool update (Quota &quota, Mdb *, mword = 0);

        static void shootdown();

        void init (Quota &quota, unsigned);

        ALWAYS_INLINE
        inline mword sticky_sub(mword s) { return s & 0x4; }
};
