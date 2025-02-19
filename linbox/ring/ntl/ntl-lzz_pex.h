/* Copyright (C) 2005 LinBox
 * Copyright (C) 2011 LinBox
 *
 *
 * Written by Daniel Roche, August 2005
 *
 * ========LICENCE========
 * This file is part of the library LinBox.
 *
  * LinBox is free software: you can redistribute it and/or modify
 * it under the terms of the  GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * ========LICENCE========
 */

/*! @file ring/ntl/ntl-lzz_pEX.h
 * @ingroup ring
 * @ingroup NTL
 * @brief NO DOC
 */

#ifndef __LINBOX_field_ntl_lzz_pEX_H
#define __LINBOX_field_ntl_lzz_pEX_H

#ifndef __LINBOX_HAVE_NTL
#error "you need NTL here"
#endif

#include <vector>
#include <NTL/lzz_pEX.h>
#include <NTL/lzz_pEXFactoring.h>

#include "linbox/linbox-config.h"
#include "linbox/util/debug.h"

#include <givaro/zring.h>
#include "linbox/ring/ntl/ntl-lzz_pe.h"
#include "linbox/integer.h"


// Namespace in which all LinBox code resides
namespace LinBox
{
	class NTL_zz_pEX_Initialiser {
	public :
		NTL_zz_pEX_Initialiser( const Integer & q, size_t e = 1) {
			if ( q > 0 )
				NTL::zz_p::init(static_cast<int64_t>(q)); // it's an error if q not prime
			NTL::zz_pX irredPoly = NTL::BuildIrred_zz_pX ((long) e);
			NTL::zz_pE::init(irredPoly);
		}

            // template <class ElementInt>
            // NTL_zz_pEX_Initialiser(const ElementInt& d) {
			// NTL::ZZ_p::init (NTL::to_ZZ(d));
            // }

		NTL_zz_pEX_Initialiser () { }

	};

        /** Ring (in fact, a unique factorization domain) of polynomial with
         * coefficients in class NTL_zz_p (integers mod a wordsize prime).
         * All the same functions as any other ring, with the addition of:
         * Coeff (type), CoeffField (type), getCoeffField, setCoeff, getCoeff,
         * leadCoeff, deg
         */
	class NTL_zz_pEX :  public NTL_zz_pEX_Initialiser, public Givaro::UnparametricOperations<NTL::zz_pEX> {
	public:
		typedef NTL::zz_pEX Element ;
		typedef Givaro::UnparametricOperations<Element> Father_t ;
		typedef UnparametricRandIter<Element> RandIter;


		typedef NTL_zz_pE CoeffField;
		typedef NTL::zz_pE Coeff;
            // typedef NTL::zz_pEX Element;

		const Element zero,one,mOne ;


            /** Standard LinBox field constructor.  The paramters here
             * (prime, exponent) are only used to initialize the coefficient field.
             */
		NTL_zz_pEX( const integer& p, int32_t e = 1 ) :
                    // Givaro::ZRing<NTL::zz_pEX>(p, e), _CField(p,e)
                NTL_zz_pEX_Initialiser(p,e),Father_t ()
			, zero( NTL::to_zz_pEX(0)),one( NTL::to_zz_pEX(1)),mOne(-one)
			, _CField(p,e)
            {}

            /** Constructor from a coefficient field */
		NTL_zz_pEX( CoeffField cf ) : Father_t ()
			,zero( NTL::to_zz_pEX(0)),one( NTL::to_zz_pEX(1)),mOne(-one)
			,_CField(cf)
            {}

		Element& init( Element& p) const
            {	return init(p, 0); }

		Element& init( Element& p, integer n ) const
            {
                p = 0;
                integer base;
                Coeff a;
                _CField.cardinality(base);
                for (int i = 0; n > 0; n /= base, ++i)
                {
                    _CField.init(a, n%base);
                    NTL::SetCoeff( p, i, a );
                }
                return p;
            }

		integer& convert( integer& n, Element& p ) const
            {
                integer base;
                integer d;
                _CField.cardinality(base);
                n = 0;
                for (int i = (int)deg(p); i >= 0; --i)
                {
                    n *= base;
                    n += _CField.convert(d, NTL::coeff(p, i));
                }
                return n;
            }

#if 0
            /** Initialize p to the constant y (p = y*x^0) */
		template <class ANY>
		Element& init( Element& p, const ANY& y = 0) const
            {
                Coeff temp;
                _CField.init( temp, y );
                return p = temp;
            }
#endif

            /** Initialize p to the constant y (p = y*x^0) */
		Element& init( Element& p, const Coeff& y ) const
            {
                return p = y;
            }

            /** Initialize p from a vector of coefficients.
             * The vector should be ordered the same way NTL does it: the front
             * of the vector corresponds to the trailing coefficients, and the back
             * of the vector corresponds to the leading coefficients.  That is,
             * v[i] = coefficient of x^i.
             */
		template <class ANY1, class ANY2, template <class T1, class T2> class Vect>
		Element& init( Element& p, const Vect<ANY1,ANY2>& v ) const
            {
                p = 0;
                Coeff temp;
                for( size_t i = 0; i < v.size(); ++i ) {
                    _CField.init( temp, v[i] );
                    if( !_CField.isZero(temp) )
                        NTL::SetCoeff( p, i, temp );
                }
                return p;
            }


            /** Initialize p from a vector of coefficients.
             * The vector should be ordered the same way NTL does it: the front
             * of the vector corresponds to the trailing coefficients, and the back
             * of the vector corresponds to the leading coefficients.  That is,
             * v[i] = coefficient of x^i.
             */
		Element& init( Element& p, const std::vector<Coeff>& v ) const
            {
                p = 0;
                for( size_t i = 0; i < v.size(); ++i )
                    NTL::SetCoeff( p, i, v[i] );
                return p;
            }

            /** Convert p to a vector of coefficients.
             * The vector will be ordered the same way NTL does it: the front
             * of the vector corresponds to the trailing coefficients, and the back
             * of the vector corresponds to the leading coefficients.  That is,
             * v[i] = coefficient of x^i.
             */
		template< class ANY >
		std::vector<ANY>& convert( std::vector<ANY>& v, const Element& p ) const
            {
                v.clear();
                ANY temp;
                for( long i = 0; i <= (long)this->deg(p); ++i ) {
                    _CField.convert( temp, NTL::coeff( p, i ) );
                    v.push_back( temp );
                }
                return v;
            }

            /** Convert p to a vector of coefficients.
             * The vector will be ordered the same way NTL does it: the front
             * of the vector corresponds to the trailing coefficients, and the back
             * of the vector corresponds to the leading coefficients.  That is,
             * v[i] = coefficient of x^i.
             */
		std::vector<Coeff>& convert( std::vector<Coeff>& v, const Element& p ) const
            {
                v.clear();
                for( long i = 0; i <= (long)this->deg(p); ++i )
                    v.push_back( NTL::coeff(p,i) );
                return v;
            }

            /** Test if an element equals zero */
		bool isZero( const Element& x ) const
            {
                return ( (this->deg(x) == 0) &&
                         ( _CField.isZero( NTL::ConstTerm(x) ) ) );
            }

            /** Test if an element equals one */
		bool isOne( const Element& x ) const
            {
                return ( (this->deg(x) == 0) &&
                         ( _CField.isOne( NTL::ConstTerm(x) ) ) );
            }

            /** Test if an element is invertible */
		bool isUnit( const Element& x ) const
            {
                return ( (this->deg(x) == 0) &&
                         ( _CField.isUnit( NTL::ConstTerm(x) ) ) );
            }

        bool isMOne (const Element& x) const
            {
                return ( (this->deg(x) == 0) &&
                         ( _CField.isMOne( NTL::ConstTerm(x) ) ) );

            }

            /** The LinBox field for coefficients */
		const CoeffField& getCoeffField() const
            { return _CField; }

            /** Get the degree of a polynomial
             * Unlike NTL, deg(0)=0.
             */
		size_t deg( const Element& p ) const
            {
                long temp = NTL::deg(p);
                if( temp == -1 ) return 0;
                else return static_cast<size_t>(temp);
            }

            /** r will be set to the reverse of p. */
		Element& rev( Element& r, const Element& p ) {
			NTL::reverse(r,p);
			return r;
		}

            /** r is itself reversed. */
		Element& revin( Element& r ) {
			return r = NTL::reverse(r);
		}

            /** Get the leading coefficient of this polynomial. */
		Coeff& leadCoeff( Coeff& c, const Element& p ) const
            {
                c = NTL::LeadCoeff(p);
                return c;
            }
		
		Element& monic(Element& r, const Element& p) const {			
			r = p;
			NTL::MakeMonic(r);
			return r;
		}
		
		Element& monicIn(Element& p) const {			
			NTL::MakeMonic(p);
			return p;
		}
		
		bool isIrreducible(const Element &x) const {
			return NTL::DetIrredTest(x);
		}
		
		void factor(std::vector<std::pair<Element, long>> &factors, const Element &f) const {
			NTL::Vec<NTL::Pair<Element, long>> factors1;
			NTL::CanZass(factors1, f);
			
			for (int i = 1; i <= factors1.length(); i++) {
				NTL::Pair<Element, long> tup = factors1(i);
				std::pair<Element, long> tmp(tup.a, tup.b);
				factors.push_back(tmp);
			}
		}
		
		void squareFree(std::vector<std::pair<Element, long>> &factors, const Element &f) const {
			NTL::Vec<NTL::Pair<Element, long>> factors1;
			NTL::SquareFreeDecomp(factors1, f);
			
			for (int i = 1; i <= factors1.length(); i++) {
				NTL::Pair<Element, long> tup = factors1(i);
				std::pair<Element, long> tmp(tup.a, tup.b);
				factors.push_back(tmp);
			}
		}

            /** Get the coefficient of x^i in a given polynomial */
		Coeff& getCoeff( Coeff& c, const Element& p, size_t i ) const
            {
                c = NTL::coeff( p, (long)i );
                return c;
            }

            /** Set the coefficient of x^i in a given polynomial */
		Element& setCoeff( Element& p, size_t i, const Coeff& c ) const
            {
                NTL::SetCoeff(p,(long)i,c);
                return p;
            }
		
		Element& pow(Element& x, const Element& a, long e) const {
			NTL::power(x, a, e);
			return x;
		}

            /** Get the quotient of two polynomials */
		Element& quo( Element& res, const Element& a, const Element& b ) const
            {
                NTL::div(res,a,b);
                return res;
            }

            /** a = quotient of a, b */
		Element& quoin( Element& a, const Element& b ) const
            {
                return a /= b;
            }

            /** Get the remainder under polynomial division */
		Element& rem( Element& res, const Element& a, const Element& b ) const
            {
                NTL::rem(res,a,b);
                return res;
            }

            /** a = remainder of a,b */
		Element& remin( Element& a, const Element& b ) const
            {
                return a %= b;
            }

            /** Get the quotient and remainder under polynomial division */
		void quorem( Element& q, Element& r,
                     const Element& a, const Element& b ) const
            {
                NTL::DivRem(q,r,a,b);
            }

		// a = b^(-1) % f
		Element& invMod(Element &a, const Element &b, const Element &f) const
		{
			NTL::InvMod(a, b, f);
			return a;
		}
		
		Element& inv( Element& y, const Element& x ) const
            {
                    // Element one(0, 1);
                return quo(y,one,x);
            }

		Element& invin( Element& y ) const
            {
                Element x = y;
                return inv(y, x);
            }
            
        bool isDivisor(const Element &x, const Element &y) const {
        	return isZero(x % y);
        }
        
        Element &gcd(Element &g, const Element &a, const Element &b) const {
        	NTL::GCD(g, a, b);
        	return g;
        }
        
        Element &gcdin(Element &g, const Element &a) const {
        	g = NTL::GCD(g, a);
        	return g;
        }

		Element& gcd( Element& res, Element& s, Element& t, const Element& a, const Element& b ) const
		{
			NTL::XGCD(res,s,t,a,b);
			return res;
		}

            /** Get characteristic of the field - same as characteristic of
             * coefficient field. */
		integer& characteristic( integer& c ) const
            { return _CField.characteristic(c); }

            /** Get the cardinality of the field.  Since the cardinality is
             * infinite, by convention we return -1.
             */
		integer& cardinality( integer& c ) const
            { return c = static_cast<integer>(-1); }

		static inline integer maxCardinality()
            { return NTL_zz_p::maxCardinality(); }
            /** Write a description of the field */
            // Oustide of class definition so write(ostream&,const Element&) from
            // Givaro::ZRing still works.

		std::ostream& write( std::ostream& os ) const
            {
                return os << "Polynomial ring using NTL::zz_pEX";
            }
            
		std::ostream& write( std::ostream& os, const Element& x) const {
			// return Father_t::write(os, x);
			if (isZero(x)) {
				os << "0";
				return os;
			}
			
			bool first = true;
			for (size_t i = 0; i <= deg(x); i++) {
				Coeff xi = NTL::coeff(x, i);
				if (!getCoeffField().isZero(xi)) {
					if (!first) {
						os << "+";
					}
					
					if (xi == 1 && i > 0) {
						os << "y";
					} else {
						getCoeffField().write(os, xi);
						if (i > 0) {
							os << "*y";
						}
					}
					
					if (i > 1) {
						os << "^" << i;
					}
					first = false;
				}
			}
			return os;
		}

            /** Conversion to scalar types doesn't make sense and should not be
             * used.  Use getCoeff or leadCoeff to get the scalar values of
             * specific coefficients, and then convert them using coeffField()
             * if needed.
             */
		template< class ANY >
		ANY& convert( ANY& x, const Element& y ) const
            { return x; }

	protected:
		CoeffField _CField;
	}; // end of class NTL_zz_pEX




	template <class Ring>
	struct ClassifyRing;

	template<>
	struct ClassifyRing<UnparametricRandIter<NTL::zz_pEX> > {
		typedef RingCategories::ModularTag categoryTag;
	};

	template<>
	class UnparametricRandIter<NTL::zz_pEX> {
	public:
		typedef NTL::zz_pEX Element;
        typedef Element::modulus_type Residu_t;

		UnparametricRandIter<NTL::zz_pEX>(const NTL_zz_pEX & F ,
                                          const uint64_t seed = 0,
                                          const size_t& size = 0
                                          ) :
                _size(size), _seed(seed), _ring(F)
            {
                if(_seed == 0)
                    NTL::SetSeed(NTL::to_ZZ(static_cast<long unsigned int>(std::time(nullptr))));
                else
                    NTL::SetSeed(NTL::to_ZZ(static_cast<long unsigned int>(_seed)));
            }

        const NTL_zz_pEX& ring() const { return _ring; }

		UnparametricRandIter<NTL::zz_pEX>(const UnparametricRandIter<NTL::zz_pEX>& R) :
                _size(R._size), _seed(R._seed), _ring(R._ring)

            {
                if(_seed == 0)
                    NTL::SetSeed(NTL::to_ZZ(static_cast<long unsigned int>(std::time(nullptr))));
                else
                    NTL::SetSeed(NTL::to_ZZ(static_cast<long unsigned int>(_seed)));
            }

		Element& random (Element& x) const
            {
                NTL::random(x, 1);
                return x;
            }

	protected:
		size_t _size;
		uint64_t _seed;
        const NTL_zz_pEX& _ring;
	}; // class UnparametricRandIters

} // end of namespace LinBox

#endif // __LINBOX_field_ntl_lzz_pEX_H

// Local Variables:
// mode: C++
// tab-width: 4
// indent-tabs-mode: nil
// c-basic-offset: 4
// End:
// vim:sts=4:sw=4:ts=4:et:sr:cino=>s,f0,{0,g0,(0,\:0,t0,+0,=s
