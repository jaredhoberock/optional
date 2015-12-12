#include <experimental/optional>
#include <cassert>
#include <iostream>

int main()
{
  using namespace std::experimental;

  // test empty construction
  optional<int> o1;
  assert(bool(!o1));

  // test value_or()
  assert(o1.value_or(13) == 13);

  // test bad_optional_access
  try
  {
    int v = o1.value();
    assert(0);
  }
  catch(bad_optional_access)
  {
  }
  catch(...)
  {
    assert(0);
  }

  // test assignment
  o1 = 13;
  assert(bool(o1));
  assert(*o1 == 13);
  assert(o1.value() == 13);

  // test valid construction
  optional<float> o2 = 7.f;
  assert(bool(o2));

  // test value_or()
  assert(o2.value_or(42.) == 7.f);

  // test copy construction
  optional<float> o3 = o2;
  assert(o3.value() == 7.f);

  std::cout << "OK" << std::endl;

  return 0;
}

