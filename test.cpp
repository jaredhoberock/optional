#include <experimental/optional>
#include <cassert>
#include <iostream>
#include <vector>

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

  // test inplace construction
  optional<std::vector<int>> o4(in_place, 10, 13);
  assert(o4.value() == std::vector<int>(10,13));

  // test inplace initializer_list construction
  optional<std::vector<int>> o5(in_place, {10,13}, std::allocator<int>());
  assert(o5.value() == std::vector<int>({10,13}, std::allocator<int>()));

  // test ==
  {
    optional<int> empty1;
    optional<int> empty2;

    assert(empty1 == empty2);
    assert(empty2 == empty1);

    auto o1 = make_optional(13);
    auto o2 = make_optional(13);
    auto o3 = make_optional(42);

    assert(o1 == o2);
    assert(!(o1 == empty1));
    assert(!(empty1 == o1));
    assert(o2 == o1);

    assert(!(o1 == o3));
    assert(!(o3 == o1));
  }

  // test <
  {
    optional<int> empty1;
    optional<int> empty2;

    assert(!(empty1 < empty2));
    assert(!(empty2 < empty1));

    auto o1 = make_optional(13);
    auto o2 = make_optional(13);
    auto o3 = make_optional(42);

    assert(!(o1 < o2));
    assert(!(o1 < empty1));
    assert(empty1 < o1);
    assert(!(o2 < o2));

    assert(o1 < o3);
    assert(!(o3 < o1));
  }

  std::cout << "OK" << std::endl;

  return 0;
}

