//#include "wvstream.h"
//#include "wvstring.h"
#include "wvxplc.h"
#include <xplc/xplc.h>
#include <xplc/IServiceManager.h>
#include <xplc/IStaticServiceHandler.h>
#include <xplc/IMonikerService.h>
#include <xplc/IMoniker.h>
#include <assert.h>
#include <stdio.h>

class Hello : public IObject
{
public:
    Hello()
        { printf("Hello!\n"); }
    virtual ~Hello()
        { printf("Goodbye!\n"); }
};

XUUID_MAP_BEGIN(Hello)
  XUUID_MAP_ENTRY(IObject)
XUUID_MAP_END


class HelloFactory : public IMoniker
{
public:
    virtual IObject *resolve(const char *s)
        { return new GenericComponent<Hello>; }
};

XUUID_MAP_BEGIN(HelloFactory)
  XUUID_MAP_ENTRY(IObject)
  XUUID_MAP_ENTRY(IMoniker)
XUUID_MAP_END



static const UUID _hellouuid =
  {0x414a69c6, 0x3c9e, 0x49f7,
      {0xab, 0x08, 0xe5, 0x5c,
	      0x7b, 0x6c, 0x23, 0x99}};


int main()
{
    fprintf(stderr, "Starting...\n");
    
    XPLC xplc;
    
    IServiceManager *servmgr = XPLC_getServiceManager();
    assert(servmgr);
    
    IStaticServiceHandler *handler = mutate<IStaticServiceHandler>(
		       servmgr->getObject(XPLC_staticServiceHandler));
    assert(handler);
    
    handler->addObject(_hellouuid, new GenericComponent<HelloFactory>);
    
    IMoniker *monikers = mutate<IMoniker>(servmgr->getObject(XPLC_monikers));
    assert(monikers);
    IMonikerService *monserv = get<IMonikerService>(monikers);
    assert(monserv);
    
    monserv->registerObject("hello", _hellouuid);
    
    assert(xplc.create<IObject>("hello:"));
    
    fprintf(stderr, "Done.\n");
    return 0;
}
