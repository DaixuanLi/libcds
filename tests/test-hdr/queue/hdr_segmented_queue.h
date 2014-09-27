//$$CDS-header$$

#ifndef __CDSHDR_QUEUE_SEGMENTED_QUEUE_H
#define __CDSHDR_QUEUE_SEGMENTED_QUEUE_H

#include "cppunit/cppunit_proxy.h"
#include <cds/intrusive/details/base.h>
#include <cds/ref.h>
#include "size_check.h"

namespace queue {

    class HdrSegmentedQueue: public CppUnitMini::TestCase
    {
        struct item {
            size_t nVal;

            item() {}
            item( size_t v ): nVal(v) {}
            item( size_t nMajor, size_t nMinor ): nVal( nMajor * 16 + nMinor ) {}
        };

        struct other_item {
            size_t  nVal;
        };

        struct push_functor {
            void operator()( item& dest, other_item const& src ) const
            {
                dest.nVal = src.nVal;
            }
        };

        struct pop_functor {
            size_t nCount;

            void operator()( other_item& dest, item const& src )
            {
                dest.nVal = src.nVal;
                ++nCount;
            }

            pop_functor()
                : nCount(0)
            {}
        };

        template <typename Queue>
        void test()
        {
            for ( size_t nQuasiFactor = 2; nQuasiFactor <= 256; ++nQuasiFactor ) {
                CPPUNIT_MSG( "QuasiFactor=" << nQuasiFactor << "..." );
                test_qf<Queue>( nQuasiFactor );
            }
        }

        template <typename Queue>
        void test_qf( size_t nQuasiFactor )
        {
            typedef typename Queue::value_type value_type;

            static size_t const c_nItemCount = 1000;

            {
                Queue q( nQuasiFactor );
                CPPUNIT_CHECK( q.quasi_factor() == cds::beans::ceil2(nQuasiFactor) );
                CPPUNIT_CHECK( q.empty() );
                CPPUNIT_CHECK( misc::check_size( q, 0 ));

                // push/enqueue
                for ( size_t i = 0; i < c_nItemCount; ++i ) {
                    if ( i & 1 ) {
                        CPPUNIT_ASSERT( q.push(item(i)) );
                    }
                    else {
                        CPPUNIT_ASSERT( q.enqueue(item(i)) );
                    }
                    CPPUNIT_CHECK( misc::check_size( q, i + 1 ));
                    CPPUNIT_CHECK( !q.empty() );
                }

                // pop/dequeue
                size_t nCount = 0;
                while ( !q.empty() ) {
                    value_type v;
                    if ( nCount & 1 ) {
                        CPPUNIT_ASSERT( q.pop( v ) );
                    }
                    else {
                        CPPUNIT_ASSERT( q.dequeue( v ));
                    }

                    int nSegment = int( nCount / q.quasi_factor() );
                    int nMin = nSegment * int(q.quasi_factor());
                    int nMax = nMin + int(q.quasi_factor()) - 1;
                    CPPUNIT_CHECK_EX( nMin <= static_cast<int>(v.nVal) && static_cast<int>( v.nVal ) <= nMax, nMin << " <= " << v.nVal << " <= " << nMax );

                    ++nCount;
                    CPPUNIT_CHECK( misc::check_size( q, c_nItemCount - nCount ));
                }
                CPPUNIT_CHECK( nCount == c_nItemCount );
                CPPUNIT_CHECK( q.empty() );
                CPPUNIT_CHECK( misc::check_size( q, 0 ));


                // push/pop with functor
                for ( size_t i = 0; i < c_nItemCount; ++i ) {
                    other_item itm;
                    itm.nVal = i;
                    if ( i & 1 ) {
                        CPPUNIT_ASSERT( q.push( itm, push_functor() ));
                    }
                    else {
                        CPPUNIT_ASSERT( q.enqueue( itm, push_functor() ));
                    }
                    CPPUNIT_CHECK( misc::check_size( q, i + 1 ));
                    CPPUNIT_CHECK( !q.empty() );
                }

                {
                    pop_functor pf;
                    other_item v;

                    nCount = 0;
                    while ( !q.empty() ) {
                        if ( nCount & 1 ) {
                            CPPUNIT_ASSERT( q.pop( v, cds::ref(pf) ));
                        }
                        else {
                            CPPUNIT_ASSERT( q.dequeue( v, cds::ref(pf) ));
                        }

                        // It is possible c_nItemCount % quasi_factor() != 0
                        // In this case the segment cannot be calculated here
                        size_t nMin = nCount > q.quasi_factor() ? nCount - q.quasi_factor() : 0;
                        size_t nMax = nCount + q.quasi_factor();
                        CPPUNIT_CHECK_EX( nMin <= v.nVal && v.nVal <= nMax, nMin << " <= " << v.nVal << " <= " << nMax );

                        ++nCount;
                        CPPUNIT_CHECK( pf.nCount == nCount );
                        CPPUNIT_CHECK( misc::check_size( q, c_nItemCount - nCount ));
                    }
                    CPPUNIT_CHECK( nCount == c_nItemCount );
                    CPPUNIT_CHECK( q.empty() );
                    CPPUNIT_CHECK( misc::check_size( q, 0 ));
                }

                //emplace
                {
                    size_t nMajor = 0;
                    size_t nMinor = 0;
                    for ( size_t i = 0; i < c_nItemCount; ++i ) {
                        CPPUNIT_CHECK( q.emplace( nMajor, nMinor ));
                        if ( nMinor  == 15 ) {
                            ++nMajor;
                            nMinor = 0;
                        }
                        else
                            ++nMinor;
                        CPPUNIT_CHECK( !q.empty() );
                    }
                    CPPUNIT_CHECK( misc::check_size( q, c_nItemCount ));

                    nCount = 0;
                    while ( !q.empty() ) {
                        value_type v;
                        if ( nCount & 1 ) {
                            CPPUNIT_ASSERT( q.pop( v ) );
                        }
                        else {
                            CPPUNIT_ASSERT( q.dequeue( v ));
                        }

                        size_t nMin = nCount > q.quasi_factor() ? nCount - q.quasi_factor() : 0;
                        size_t nMax = nCount + q.quasi_factor();
                        CPPUNIT_CHECK_EX( nMin <= v.nVal && v.nVal <= nMax, nMin << " <= " << v.nVal << " <= " << nMax );

                        ++nCount;
                        CPPUNIT_CHECK( misc::check_size( q, c_nItemCount - nCount ));
                    }
                    CPPUNIT_CHECK( nCount == c_nItemCount );
                    CPPUNIT_CHECK( q.empty() );
                    CPPUNIT_CHECK( misc::check_size( q, 0 ));
                }

                // pop from empty queue
                {
                    value_type v;
                    v.nVal = c_nItemCount + 1;
                    CPPUNIT_CHECK( q.empty() );
                    CPPUNIT_ASSERT( !q.pop( v ));
                    CPPUNIT_CHECK( q.empty() );
                    CPPUNIT_CHECK( misc::check_size( q, 0 ));
                    CPPUNIT_CHECK( v.nVal == c_nItemCount + 1 );
                }

                // clear
                for ( size_t i = 0; i < c_nItemCount; ++i ) {
                    if ( i & 1 ) {
                        CPPUNIT_ASSERT( q.push(item(i)) );
                    }
                    else {
                        CPPUNIT_ASSERT( q.enqueue(item(i)) );
                    }
                    CPPUNIT_CHECK( misc::check_size( q, i + 1 ));
                    CPPUNIT_CHECK( !q.empty() );
                }

                q.clear();
                CPPUNIT_CHECK( misc::check_size( q, 0 ));
                CPPUNIT_CHECK( q.empty() );
            }
        }

        void SegmQueue_HP();
        void SegmQueue_HP_mutex();
        void SegmQueue_HP_shuffle();
        void SegmQueue_HP_stat();

        void SegmQueue_PTB();
        void SegmQueue_PTB_mutex();
        void SegmQueue_PTB_shuffle();
        void SegmQueue_PTB_stat();

        CPPUNIT_TEST_SUITE(HdrSegmentedQueue)
            CPPUNIT_TEST( SegmQueue_HP )
            CPPUNIT_TEST( SegmQueue_HP_mutex )
            CPPUNIT_TEST( SegmQueue_HP_shuffle )
            CPPUNIT_TEST( SegmQueue_HP_stat )

            CPPUNIT_TEST( SegmQueue_PTB )
            CPPUNIT_TEST( SegmQueue_PTB_mutex )
            CPPUNIT_TEST( SegmQueue_PTB_shuffle )
            CPPUNIT_TEST( SegmQueue_PTB_stat )
        CPPUNIT_TEST_SUITE_END()

    };
} // namespace queue

#endif //#ifndef __CDSHDR_QUEUE_SEGMENTED_QUEUE_H
