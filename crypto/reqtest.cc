#include "wvx509.h"
#include "wvlog.h"

// Quick program to test the certificate request generation routines
// from WvX509Mgr. Output should be put through:
// openssl req -text
// (The part between ----BEGIN CERTIFICATE REQUEST---- and 
// ----END CERTIFICATE REQUEST---- )

int main(int argc, char **argv)
{
    free(malloc(1)); // For Electric Fence...
    
    WvString request;
    
    WvLog log("reqtest", WvLog::Info);
    log("Starting...\n");
    
    // Setup a new DN entry, like a server would set.
    WvString dn("cn=test.foo.com,dc=foo,dc=com");
    
    // Create a new certificate
    WvX509Mgr cert(dn, 1024);
    
    if (!cert.isok())
    {
	log("Failed to generate certificate: %s\n", cert.errstr());
	return 1;
    }
    
    wvcon->write(cert.encode(WvX509Mgr::RsaPEM));
    wvcon->write(cert.certreq());
    log("Done...\n");
}
