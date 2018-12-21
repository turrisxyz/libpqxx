#include <cerrno>
#include <cstring>
#include <iostream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Mixed-mode, seeking test program for libpqxx's Large Objects interface.
namespace
{
const string Contents = "Large object test contents";


class WriteLargeObject : public transactor<>
{
public:
#include <pqxx/internal/ignore-deprecated-pre.hxx>
  explicit WriteLargeObject() :
    transactor<>("WriteLargeObject")
  {
  }
#include <pqxx/internal/ignore-deprecated-post.hxx>

  void operator()(argument_type &T)
  {
    largeobjectaccess A(T);
    cout << "Writing to large object #" << largeobject(A).id() << endl;

    A.write(Contents);

    char Buf[200];
    const size_t Size = sizeof(Buf) - 1;
    PQXX_CHECK_EQUAL(
	A.read(Buf, Size),
	0,
	"Could read bytes from large object after writing.");

    // Overwrite terminating zero
    auto Here = A.seek(-1, ios::cur);

    PQXX_CHECK_EQUAL(
	Here,
	largeobject::size_type(Contents.size()-1),
	"Ended up in wrong place after moving back 1 byte.");

    A.write("!", 1);

    // Now check that we really did
    PQXX_CHECK_EQUAL(
	A.seek(-1, ios::cur),
	largeobject::size_type(Contents.size()-1),
	"Inconsistent seek.");

    char Check;
    PQXX_CHECK_EQUAL(
	A.read(&Check, 1),
	1,
	"Unexpected result from read().");
    PQXX_CHECK_EQUAL(string(&Check, 1), "!", "Read back wrong character.");

    PQXX_CHECK_EQUAL(
	A.seek(0, ios::beg),
	0,
	"Ended up in wrong place after seeking back to beginning.");

    PQXX_CHECK_EQUAL(
	A.read(&Check, 1),
	1,
	"Unexpected result when trying to read back 1st byte.");

    PQXX_CHECK_EQUAL(
	string(&Check, 1),
	string(Contents, 0, 1),
	"Wrong first character in large object.");

    // Clean up after ourselves
    A.remove(T);
  }
};


void test_058(transaction_base &orgT)
{
  connection_base &C(orgT.conn());
  orgT.abort();

#include <pqxx/internal/ignore-deprecated-pre.hxx>
  C.perform(WriteLargeObject());
#include <pqxx/internal/ignore-deprecated-post.hxx>
}
} // namespace

PQXX_REGISTER_TEST_T(test_058, nontransaction)
