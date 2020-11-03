#include "enumset.h"
#include <vector>

#ifdef XP_TESTING
#include "checkunit.h"

void init_enumset_tests() {}

enum class E { A, B, C, D, E, F };

START_TEST(enumset)
{
    // empty set
    {
        const EnumSet<E> s;
        fail_unless(s.empty());
        fail_unless(s.size() == 0);
        fail_unless(s.begin() == s.end());
    }

    // ctor from iterator range
    {
        const std::vector<E> v = { E::A, E::B };
        const EnumSet<E> s(v.begin(), v.end());
        fail_unless(s.is_equal_to({ E::A, E::B }));
    }

    // serialization / deserialization
    {
        const EnumSet<E> s = { E::A, E::B, E::F };
        fail_unless(s.deserialize_ull(s.serialize_ull()) == s);
        fail_unless(s.deserialize_str(s.serialize_str()) == s);
    }

    // initializer list, size, contains, count
    {
        const EnumSet<E> s { E::A, E::B, E::C };
        fail_if(s.empty());
        fail_unless(s.size() == 3);
        fail_unless(s == EnumSet<E>({ E::A, E::B, E::C }));
        fail_unless(s != EnumSet<E>({ E::A, E::B }));
        fail_unless(s < EnumSet<E>({ E::F }));
        fail_unless(s.contains(E::A));
        fail_if(s.contains(E::D));
        fail_unless(s.count({ E::A, E::B, E::D }) == 2);
    }

    // initializer list 2
    {
        EnumSet<E> s = { E::A, E::B, E::C };
        fail_unless(s.is_equal_to({ E::A, E::B, E::C }));
        s = { E::D, E::F };
        fail_unless(s.is_equal_to({ E::D, E::F }));
    }

    // insert
    {
        EnumSet<E> s;
        s.insert(E::A);
        s.insert({ E::A, E::B });
        fail_unless(s.size() == 2);
        fail_unless(s.contains(E::A));
        fail_unless(s.contains(E::B));
        fail_unless(s.is_equal_to({ E::A, E::B }));
    }

    // insert 2
    {
        const std::vector<E> v = { E::B, E::C };
        EnumSet<E> s = { E::A, E::B };
        s.insert(v.begin(), v.end());
        fail_unless(s.is_equal_to({ E::A, E::B, E::C }));
    }

    // erase
    {
        EnumSet<E> s { E::A, E::B, E::C, E::D, E::E, E::F };
        s.erase(E::A);
        s.erase(E::A);
        s.erase(E::B);
        s.erase({ E::E, E::F });
        fail_unless(s.is_equal_to({ E::C, E::D }));
    }

    // erase 2
    {
        const std::vector<E> v = { E::B, E::C };
        EnumSet<E> s = { E::A, E::B, E::C, E::D };
        s.erase(v.begin(), v.end());
        fail_unless(s.is_equal_to({ E::A, E::D }));
    }

    // subset
    {
        fail_unless( EnumSet<E> {}.is_subset_of({ E::A }) );
        fail_unless( EnumSet<E> { E::A }.is_subset_of({ E::A }) );
        fail_unless( EnumSet<E> { E::A }.is_subset_of({ E::A, E::B }) );
        fail_if( EnumSet<E> { E::A }.is_subset_of({ E::B, E::C }) );
    }

    // superset
    {
        fail_unless( EnumSet<E> { E::A }.is_superset_of({}) );
        fail_unless( EnumSet<E> { E::A }.is_superset_of({ E::A }) );
        fail_unless( EnumSet<E>({ E::A, E::B }).is_superset_of({ E::A }) );
        fail_if( EnumSet<E> { E::A }.is_superset_of({ E::A, E::B, E::C }) );
    }

    // union, intersection, difference
    {
        fail_unless( EnumSet<E>::set_union( { E::A, E::B }, { E::A, E::C} )
                .is_equal_to({ E::A, E::B, E::C }));
        fail_unless( EnumSet<E>::set_intersection( { E::A, E::B }, { E::A, E::C} )
                .is_equal_to({ E::A }));
        fail_unless( EnumSet<E>::set_difference( { E::A, E::B }, { E::A, E::C} )
                .is_equal_to({ E::B }));
    }

    // iteration and range-based for
    {
        const EnumSet<E> s { E::A, E::B, E::C };
        EnumSet<E> t;
        for (EnumSet<E>::const_iterator it = s.begin(); it != s.end(); ++it) {
            t.insert(*it);
        }
        fail_unless(s == t);

        t.clear();
        fail_unless(t.empty());
        for (E item : s)
            t.insert(item);
        fail_unless(s == t);
    }
}
END_TEST;

#define SUITENAME "Serverlib"
TCASEREGISTER(0, 0)
{
    ADD_TEST(enumset);
}
TCASEFINISH;

#endif // #ifdef XP_TESTING
