/*
 * Worldvisions Weaver Software:
 *   Copyright (C) 1997-2002 Net Integration Technologies, Inc.
 *
 * UniConfClient is a UniConfGen for retrieving data from the UniConfDaemon.
 *
 * see "uniconfclient.h"
 */

#include <uniconfclient.h>

UniConfClient::UniConfClient(UniConf *_top, UniConfConnFactory *_fctry) :
    top(_top), fctry(_fctry), log("UniConfClient"), dict(5), references(0)
{
    conn = fctry->open();
    waitforsubt = false;
}

UniConfClient::~UniConfClient()
{
    conn->close();
    delete conn;
}

void UniConfClient::savesubtree(UniConf *tree, UniConfKey key)
{
    if (!conn || !conn->isok())
        return;

    if (tree->dirty && !tree->obsolete)
    {
        WvString data("set %s %s\n", wvtcl_escape(key), wvtcl_escape(*tree));
        conn->print(data);
    }
    
    // What about our children.. do we have dirty children?
    if (tree->child_dirty)
    {
	UniConf::Iter i(*tree);

        for (i.rewind(); i.next();)
        {
            if (i->generator && this != i->generator)
                continue;

            if (i->dirty || i->child_dirty)
            {
                UniConfKey key2(key);
                key2.append(&i->name, false);
                savesubtree(i.ptr(), key2);
            }
        }
    }
    // done.
}

void UniConfClient::save()
{
    // Make sure we actually need to save anything...
    if (!top->dirty && !top->child_dirty)
        return;

    // check our connection...
    if (!conn || !conn->isok())
    {
        log(WvLog::Debug2, "Connection was unuseable.  Creating another.\n");
        conn = fctry->open();
        if (!conn->isok()) // we're borked
        {
            log(WvLog::Error, "Unable to create new connection.  Save aborted.\n");
            return;
        }
    }
    
    if (conn->select(0, true, false, false))
        execute();
    // working.. yay, great, good.  Now, ship this subtree off to savesubtree
    savesubtree(top, "/");
}

UniConf *UniConfClient::make_tree(UniConf *parent, const UniConfKey &key)
{
   // Now, do a get on the key from the daemon
    WvString newkey(key);
    UniConf *par = parent;
    while (par)
    {
        if (par->name != "")
            newkey = WvString("%s/%s", par->name, newkey);
        par = par->parent;
    }
    // Get the node which we're actually going to return...
    UniConf *toreturn = UniConfGen::make_tree(parent, key);
    // Now wait for the response regarding this key.
    if (toreturn->waiting)
        update(toreturn);
    return toreturn;
}

void UniConfClient::enumerate_subtrees(UniConf *conf)
{
    if (!conn || !conn->isok())
        return;
    
    if (conn->select(0, true, false, false))
        execute();
    if (conf != top)
        conn->print(WvString("subt %s\n", wvtcl_escape(conf->full_key(top))));
    else
        conn->print(WvString("subt /\n"));
    waitforsubt = true;
    while (waitforsubt && conn->isok())
    {
        if (conn->select(0, true, false, false))
            execute();
    }
}

void UniConfClient::update(UniConf *&h)
{
    WvString lookfor(h->full_key(top));
    if (h->waiting || h->obsolete)
    {
        if (conn && conn->isok())
            conn->print(WvString("get %s\n", wvtcl_escape(lookfor)));
        else
        {
            h->waiting = false;
            return;
        }
    }
    waitingdata *data = dict[lookfor];

//    wvcon->print("Looking for:  %s.\n", lookfor);
    
    if (conn->select(0,true, false, false) 
    || (h->waiting && !data && conn->select(-1, true, false, false)))
    {
        execute();
        data = dict[lookfor];
    }
    
    if (data) 
    {
        // If we are here, we will not longer be waiting nor will our data be
        // obsolete.
        h->set(data->value.unique());
//        dict.remove(data);
    }

    h->waiting = false;
    h->obsolete = false;
    h->dirty = false;
}

bool UniConfClient::deleteable()
{
    references--;
    return 0 == references;
}

void UniConfClient::execute()
{
    conn->fillbuffer();
    for (;;)
    {
        WvString line = conn->gettclline();
        if (line.isnull())
            break;
        WvConstStringBuffer fromline(line);
        for (;;)
        {
            WvString cmd = wvtcl_getword(fromline);
            WvString key = wvtcl_getword(fromline);
            if (cmd.isnull() || key.isnull())
                break;
            
            // Value from a get is incoming
            if (cmd == "RETN") 
            {
                WvString value = wvtcl_getword(fromline);
                dict.add(new waitingdata(key.unique(), value.unique()),
                    true);
//                wvcon->print("Added key(%s) to dict with value(%s).\n", key, value);
            }
            // A set has happened on a key we requested.
            else if (cmd == "FGET") 
            {
                dict.remove(dict[key]);
                UniConf *obs = &(*top)[key];
                if (obs)
                {
                    obs->obsolete = true;
                    UniConf *par = obs->parent;
                    while (par)
                    {
                        par->child_obsolete = true;
                        par = par->parent;
                    }
                }
            }
            else if (cmd == "SUBT")  // This is so inefficient it scares me.
            {
                waitforsubt = false;
                while (fromline.used() > 0)
                {
                    WvString pair = wvtcl_getword(fromline);
                    WvDynamicBuffer temp;
                    temp.putstr(pair);
                    WvString newkey = wvtcl_getword(temp);
                    WvString newval = wvtcl_getword(temp);
                    dict.add(new waitingdata(newkey.unique(),
                        newval.unique()), false);
                    UniConf *narf = &top->get(key);
                    narf = &narf->get(newkey);
                    narf->generator = this;
                }
            }
        }
    }
}


