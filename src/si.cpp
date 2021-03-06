/*
 * Signal
 *
 * Copyright (C) 2014-2015 Alexander Boettcher, Genode Labs GmbH.
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

#include "si.hpp"
#include "sm.hpp"
#include "assert.hpp"

#include "stdio.hpp"

static Spinlock lock;

Si::Si (Sm * s, mword v) : sm(s), prev(nullptr), next(nullptr), value(v)
{
    trace (TRACE_SYSCALL, "SI:%p created (SM:%p signal:%#lx)", this, s, v);

    if (sm)
        sm->add_ref();
}

Si::~Si()
{
    if (!sm)
        return;

    if (queued()) {
        bool r = sm->Queue<Si>::dequeue(this);
        assert(r);
    }

    if (sm->del_ref()) {
        Pd *pd = static_cast<Pd *>(static_cast<Space_obj *>(sm->space));
        Sm::destroy(sm, pd->quota);
    }
}

void Si::chain(Sm *si)
{
    Sm * kern_sm = static_cast<Sm *>(this);
    assert (kern_sm);
    assert (kern_sm->space == static_cast<Space_obj *>(&Pd::kern));

    Lock_guard <Spinlock> guard (lock);

    if (sm && sm->del_ref()) {
        sm->add_ref();
        sm->call_rcu();
    }

    sm = si;

    if (sm)
        sm->add_ref();

    mword c = kern_sm->reset(true);

    for (unsigned i = 0; i < c; i++)
        kern_sm->submit();
}

void Si::submit()
{
    Sm * i = static_cast<Sm *>(this);
    assert(i);

    if (i->space == static_cast<Space_obj *>(&Pd::kern)) {
        Sm * si = ACCESS_ONCE(i->sm);
        if (si) {
            si->submit();
            return;
        }
    }

    i->up();

    Sm * sm_chained = ACCESS_ONCE(sm);
    /* if !sm than it is just a semaphore */
    if (!sm_chained) return;

    /* signal mode - send up() to chained sm */
    sm_chained->up(nullptr, i);
}
