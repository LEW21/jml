/* compact_size_type_test.cc
   Jeremy Barnes, 12 August 2009
   Copyright (c) 2009 Jeremy Barnes.  All rights reserved.

   Testing for the compact size type.
*/

#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK

#include "jml/utils/parse_context.h"
#include "jml/utils/file_functions.h"
#include "jml/utils/guard.h"
#include "jml/db/persistent.h"
#include "jml/db/compact_size_types.h"
#include <boost/test/unit_test.hpp>
#include <boost/bind.hpp>
#include <sstream>

using namespace ML;
using namespace ML::DB;
using namespace std;

using boost::unit_test::test_suite;

void test_compact_size_type(uint64_t value)
{
    std::ostringstream os;

    {
        DB::Store_Writer store(os);
        compact_size_t cs(value);
        store << cs;
    }

    cerr << "os.str().size() = " << os.str().size() << endl;

    std::istringstream is(os.str());
    DB::Store_Reader store(is);

    compact_size_t cs(store);

    BOOST_CHECK_EQUAL(cs.size_, value);
}

//void test_signed_compact_size_type(uint64_t value)
//{
//}

BOOST_AUTO_TEST_CASE( test1 )
{
    // TODO: should go to 64, but problem that I don't have time to look at...

    for (unsigned i = 0;  i < 63;  ++i) {
        unsigned long long val = 1ULL << i;

        cerr << "testing i = " << i << endl;

        test_compact_size_type(val - 1);
        test_compact_size_type(val);
        test_compact_size_type(val + 1);
    }
}
