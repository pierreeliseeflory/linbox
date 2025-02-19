/* Copyright (C) LinBox
 *  Author: Zhendong Wan
 *  Modified by Pascal Giorgi
 *
 *
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



/* Reserve for possible optimal.
*/

#ifndef __LINBOX_apply_H
#define __LINBOX_apply_H

#include "linbox/linbox-config.h"
#include "givaro-config.h"
#include "linbox/integer.h"
#include "linbox/util/debug.h"
// #if defined(__LINBOX_field_multimod_field_H) && !defined(__LINBOX_blas_matrix_domain_H)
// #error "you need to include \"multimod-field.h\" before \"blas-domain.h\""
// #endif
#include "linbox/field/multimod-field.h"
#include "linbox/field/hom.h"
#include "linbox/randiter/multimod-randomprime.h"
#include "linbox/matrix/sparse-matrix.h"
#include "linbox/matrix/dense-matrix.h"
#include "linbox/algorithms/lifting-container.h"
#include <vector>
#include "linbox/vector/blas-vector.h"


#include "linbox/util/timer.h"


// #ifdef __LINBOX_BLAS_AVAILABLE
#include <fflas-ffpack/fflas/fflas.h>
// #endif

//#define DEBUG_CHUNK_SETUP
//#define DEBUG_CHUNK_APPLY
//#define DEBUG_CHUNK_APPLYM
//#define CHECK_APPLY
#define TIMING_APPLY

namespace LinBox
{

	// general case, y = A x
	template<class OutV, class Matrix, class InV>
	inline OutV& apply (OutV& y, const Matrix& A, const InV& x)
	{

		return A. apply (y, x);

	}

	template<class OutV, class Matrix, class InV>
	inline OutV& applyTranspose (OutV& y, const Matrix& A, const InV& x)
	{

		return A. applyTranspose (y, x);

	}


	template<class Domain>
	class BlasApply {

	public:
		typedef typename Domain::Element   Element;
		typedef BlasVector<Domain>         Vector;

		BlasApply(const Domain& D) :
			_domain(D), _MD(D)
		{
			_domain.characteristic(_prime);
		}


		//#ifdef __LINBOX_BLAS_AVAILABLE
		inline Vector& applyV(Vector                        &y,
				      const BlasMatrix<Domain>     &A,
				      const Vector                  &x) const
		{

			if (( _prime > 0) && ( _prime <  67108863)) {

				FFLAS::fgemv(_domain, FFLAS::FflasNoTrans,
					      A.rowdim(), A.coldim(),
					      _domain.one,
					      A.getPointer(), A.getStride(),
					      &x[0],1,
					      _domain.zero,
					      &y[0],1);
			}
			else {
				_MD.vectorMul (y, A, x);
			}
			return y;
		}

		inline Vector& applyVTrans(Vector                        &y,
					   const BlasMatrix<Domain>           &A,
					   const Vector                  &x) const
		{

			if (( _prime > 0) && ( _prime <  67108863)) {

				FFLAS::fgemv(_domain, FFLAS::FflasTrans,
					      A.rowdim(), A.coldim(),
					      _domain.one,
					      A.getPointer(), A.getStride(),
					      &x[0],1,
					      _domain.zero,
					      &y[0],1);
			}
			else {
				TransposeMatrix<const BlasMatrix<Domain> > B(A);
				_MD.vectorMul (y, B, x);
			}
			return y;
		}

		inline Vector& applyVspecial (Vector                        &y,
					      BlasMatrix<Domain>            &A,
					      const Vector                  &x) const
		{//toto

			size_t m,n;
			m = A.rowdim();
			n = A.coldim();
			linbox_check( x.size() == n);

			double * At_dbl = new double[m*n];

			for (size_t i=0;i<m;++i)
				for (size_t j=0;j<n;++j)
					_domain.convert(*(At_dbl+i+j*m), A.refEntry(i,j));

			integer tmp;
			bool use_neg=false;
			uint32_t maxword=0;
			for (size_t i=0;i<n;++i){
				_domain.convert(tmp,x[i]);
				if (tmp <0)
					use_neg = true;
				if ( maxword < tmp.size())
					maxword= tmp.size();
			}

			if (use_neg)
				maxword++;
			double *xdbl= new double[n*maxword];
			memset(xdbl, 0, sizeof(double)*n*maxword);

			for (size_t i=0;i<n;++i){
				_domain.convert(tmp,x[i]);
				double * ptr= xdbl+i;
				if (tmp == 0)
					*ptr=0;
				else {
					if (tmp > 0) {
						for (size_t j=0;j<tmp.size();++j){
							*ptr= (double)x[i][j];
							ptr+= n;
						}
					}
					else {
						size_t j=0;
						for (;j<tmp.size();++j){
							*ptr= double(0xFFFFFFFF^x[i][j]);
							ptr+= n;
						}
						for (;j<maxword-1;++j){
							*ptr= double(0xFFFFFFFF);
							ptr+= n;
						}
						*ptr=1;
					}
				}
			}


			double *ydbl= new double[maxword*m];

			cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
				    (int) maxword,(int) m,(int) n, 1,
				    xdbl,(int) n, At_dbl, (int) m, 0, ydbl, (int) m);

			delete[] At_dbl;
			delete[] xdbl;

			size_t rclen=maxword*4+5;
			unsigned char* combined1 = new unsigned char[m*rclen];
			unsigned char* combined2 = new unsigned char[m*rclen];
			memset(combined1, 0, m*rclen);
			memset(combined2, 0, m*rclen);

			for (size_t i=0;i<m;++i){
				unsigned char *ptr= combined1+i*rclen;
				for (size_t j=0;j< maxword;j=j+2){
					if (!use_neg || j< maxword-1){
						long long mask = static_cast<long long>(ydbl[j*m+i]);
						long long tmp;
						memcpy(&tmp,ptr,sizeof(long long));
						tmp |= mask;
						memcpy(ptr,&tmp,sizeof(long long));
						ptr+=4;
					}
				}
				ptr= combined2+4+i*rclen;
				for (size_t j=1;j< maxword;j=j+2){
					if (!use_neg || j< maxword-1){
						long long mask = static_cast<long long>(ydbl[j*m+i]);
						long long tmp;
						memcpy(&tmp,ptr,sizeof(long long));
						tmp |= mask;
						memcpy(ptr,&tmp,sizeof(long long));
						ptr+=4;
					}
				}
			}

			for (size_t i=0; i<m; i++) {
				LinBox::integer result, tmp2;
				if (use_neg) {
					result = -ydbl[(maxword-1)*m+i];
					result <<= (maxword-1)*32;
				}
				else
					result = 0;

				Givaro::Protected::importWords(tmp2, (size_t)rclen, -1, 1, 0, 0, combined1+i*rclen);
				result += tmp2;
				Givaro::Protected::importWords(tmp2, (size_t)rclen, -1, 1, 0, 0, combined2+i*rclen);
				result += tmp2;

				_domain.init(y[i], result);
			}
			delete[] ydbl;
			delete[] combined1;
			delete[] combined2;

			return y;
		}// end of applyVspecial


	private:
		Domain            _domain;
		integer            _prime;
		MatrixDomain<Domain>  _MD;


	};


	template <class Domain, class IMatrix>
	class MatrixApplyDomain {

	public:
		typedef typename Domain::Element   Element;
		typedef BlasVector<Domain>         Vector;

		MatrixApplyDomain(const Domain& D, const IMatrix &Mat) :
			_domain(D), _matM(Mat)
		{}

		void setup(LinBox::integer prime){}

		Vector& applyV(Vector& y, Vector& x, Vector& z) const
		{
			return _matM.apply(y,x);
		}

		Vector& applyVTrans(Vector& y, Vector& x, Vector&z) const
		{return _matM.applyTranspose(y,x);}

	private:
		Domain          _domain;
		const IMatrix  &_matM;
	};


	// special function to split an integer matrix in q-adic representation in an array of double
	template <class Domain, class IMatrix>
	void create_MatrixQadic (const Domain &D, const IMatrix &Mat, double *chunks, size_t num_chunks, const integer shift=0);


	// special function to split an integer vector in q-adic representation in an array of double
	template <class Domain, class IVector>
	void create_VectorQadic (const Domain &D, const IVector &V, double *chunks, size_t num_chunks);

	// special function to split an integer matrix in an RNS representation in an array of double
	template <class Domain, class IMatrix>
	void create_MatrixRNS (const MultiModDouble& F, const Domain &D, const IMatrix &Mat, double *chunks);


	// special function to split an integer vector in an RNS representation in an array of double
	template <class Domain, class IVector>
	void create_VectorRNS (const MultiModDouble& F, const Domain &D, const IVector &V, double *chunks);



	// \brief optimizations for applying an integer matrix to a bounded integer vector
	template <class Domain, class IMatrix>
	class BlasMatrixApplyDomain {

		enum ApplyChoice {Classic, MatrixQadic, VectorQadic, CRT};

	public:
		typedef typename Domain::Element   Element;
		typedef BlasVector<Domain>         Vector;
		typedef IMatrix                      Matrix;


		BlasMatrixApplyDomain(const Domain& D, const IMatrix &Mat) :
			_domain(D), _matM(Mat), _MD(D), _m(Mat.rowdim()), _n(Mat.coldim())
			,use_chunks(false),use_neg(false),chunk_size(0)
			,num_chunks(0)
			,chunks(NULL),vchunks(NULL)
		{
			_switcher= Classic;_rns=NULL;
		}


		~BlasMatrixApplyDomain ()
		{
			if (_switcher==MatrixQadic) delete[] chunks;
			if (_switcher==VectorQadic) {delete[] chunks;delete[] vchunks;}
			if (_switcher== CRT) delete _rns;
			//std::cout<<"time convert data = "<<_convert_data<<std::endl;
			//std::cout<<"time apply   = "<<_apply<<std::endl;
			//std::cout<<"time convert result = "<<_convert_result<<std::endl;
		}

		ApplyChoice  setup(LinBox::integer prime)
		{ //setup

			_domain.init(_prime,prime);
			_apply.clear();
			_convert_data.clear();
			_convert_result.clear();
#ifdef __LINBOX_HAVE_BIG_ENDIAN
			_switcher= Classic;
#else
			// compute the magnitude in bit of the matrix
			// check if at least one entry in the matrix is negative
			LinBox::integer tmp=0, maxValue=0;
			size_t maxBitSize = 0;
			use_neg = false;
			typename Matrix::ConstIterator it = _matM.Begin();
			for (size_t i=0; i<_m*_n; i++, ++it) {
				_domain.convert(tmp, *it);
				if (tmp <0) {
					use_neg = 1;
					tmp=-tmp;
				}
				if (tmp> maxValue){
					maxValue= tmp;
				}
			}
			size_t bit, dbit;
			bit=maxValue.bitsize();
			dbit= maxValue.size_in_base(4)*2;
			maxBitSize= ((dbit-bit)>0)? dbit: bit;

			// Check Qadic matrix reprentation possibility
			LinBox::integer maxChunkVal = 1;
			maxChunkVal <<= 53;
			maxChunkVal /= (prime-1) * uint64_t(_n);
			chunk_size = maxChunkVal.bitsize();
			use_chunks = (chunk_size >= 16);
			//std::cout<<"max bit= "<<maxBitSize<<" "<<maxValue.size_in_base(4)*2<<"\n";std::cout<<"max value= "<<maxValue<<"\n";
			if (use_chunks){//std::cout<<"Matrix Qadic\n";
				_switcher= MatrixQadic;

			}
			else {
				// Check Qadic vector representation possibility
				maxChunkVal = 1;
				maxChunkVal <<= 53;
				maxChunkVal /= 2*maxValue * uint64_t(_n);
				chunk_size = maxChunkVal.bitsize();
				use_chunks = (chunk_size >= 16);

				if (use_chunks){//std::cout<<"Vector Qadic\n";
					_switcher= VectorQadic;

					//std::cout<<"possible chunk size: "<<chunk_size<<std::endl;
				}
				else {
					if (prime.bitsize()> 32){//std::cout<<"CRT\n";
						_switcher= CRT;

					}
					else {//std::cout<<"Classic\n";
						_switcher= Classic;

					}
				}
			}

			// set maximum size of chunk to 16
			chunk_size = 16;

			switch (_switcher) {

			case MatrixQadic:
				if (use_neg){
					maxValue= maxValue<<1;
					maxBitSize+=1;
				}
				// compute the number of chunk
				if (maxValue*prime* uint32_t(_matM.coldim()) < integer("9007199254740992")){
					num_chunks=1;
					use_neg=false;
				}
				else num_chunks =(maxBitSize) / chunk_size+ (((maxBitSize % chunk_size) > 0)? 1:0);
				//num_chunks = 1;
				if (num_chunks ==1) use_neg= false;

				//if (use_neg) num_chunks++; //the leading chunk will be negative

				//int n2 = _m*_n;
				chunks = new double[_m*_n*num_chunks];
				memset(chunks, 0, sizeof(double)*_m*_n*num_chunks);

				shift= use_neg? maxValue : integer(0);

				create_MatrixQadic(_domain, _matM, chunks, num_chunks, shift);

#ifdef DEBUG_CHUNK_SETUP
				std::cout<<std::endl;
				std::cout<<"max bit= "<<maxBitSize<<std::endl;
				std::cout << num_chunks << " chunks of "<< chunk_size << " bits each" << std::endl;
				if (!use_neg) std::cout << "not ";
				std::cout << "using negative leading chunk" << std::endl;
				std::cout << "shift is : "<<shift<<std::endl;
				std::cout << "Contents of chunks: " << std::endl;
				for (size_t i=0; i<num_chunks; i++) {
					std::cout << "chunk " << i << std::endl;
					for (size_t j=0; j<_m*_n; j++) {
						std::cout << static_cast<long long>(chunks[i*_m*_n+j]);
						if ((j+1)%_n) std::cout << ' '; else std::cout << std::endl;
					}
				}
#endif

				break;

			case VectorQadic:
				num_chunks = (prime.bitsize() / chunk_size)+ (((prime.bitsize() % chunk_size) > 0)? 1:0);
				// convert integer matrix to double matrix
				chunks  = new double[_m*_n];
				memset(chunks, 0, sizeof(double)*_m*_n);
				create_MatrixQadic (_domain, _matM, chunks, 1);

				// if the matrix has negative entries
				if (use_neg){
					shift=maxValue;
					double sh= (double) maxValue;
					// shift the value of the entries in the matrix by ||A||=max(A_ij)
					for (size_t i=0;i<_m*_n;++i)
						chunks[i]+=sh;

				}

				// allocate memory for the vector chunks
				vchunks = new double[_n*num_chunks];
				break;

			case Classic:
				break;

			case CRT:
				if (use_neg){
					maxValue= maxValue<<1;
					// maxBitSize+=1;
				}
				integer a_bound= maxValue*uint64_t(_n)+1;
				integer b_bound= sqrt(integer("9007199254740992")/uint64_t(_n));	std::cout<<"max prime: "<<b_bound<<" max rns: "<<a_bound<<std::endl;
				MultiModRandomPrime mmrp;
				std::vector<integer> rns_basis = mmrp.createPrimes(b_bound, a_bound);
				_rns = new MultiModDouble(rns_basis);

				std::cout<<" CRT basi length= "<<_rns->size()<<std::endl;

				// convert integer matrix to rns double matrix
				chunks  = new double[_m*_n*_rns->size()];
				memset(chunks, 0, sizeof(double)*_m*_n*_rns->size());
				create_MatrixRNS(*_rns, _domain, _matM, chunks);

				// allocate memory for the rns vector
				vchunks = new double[_n*_rns->size()];

				// prepare special CRT
				Element g, s, q, two;
				_q= _rns->getCRTmodulo();
				_domain.init(q,_q);_domain.init(two,int64_t(2));
				_domain.gcd(g, _inv_q, s, q, _prime);
				if (_domain.compare(_inv_q, _domain.zero)<0 ) _domain.addin(_inv_q,_prime);
				_domain.mul(_pq,_prime,q);
				_domain.sub(_h_pq,_pq, _domain.one);
				_domain.divin(_h_pq, two);

				break;

			}

#endif
#ifdef DEBUG_CHUNK_APPLY
			std::cout<<"A: \n";
			_matM.write(std::cout);
#endif

			return _switcher;
		}


		//#define DEBUG_CHUNK_APPLY
		Vector& applyV(Vector& y, Vector& x, Vector &b) const
		{ //applyV

#ifdef DEBUG_CHUNK_APPLY
			std::cout << "x: ";
			for (size_t i=0; i<x.size(); i++)
				std::cout << x[i] << ' ';
			std::cout << std::endl;
#endif
			linbox_check( _n == x.size());
			linbox_check( _m == y.size());

			switch(_switcher) {//switch

			case Classic:
				_MD.vectorMul (y, _matM, x);
				break;

			case MatrixQadic:
				{// mqadic
#if 0
					temp fix
					_MD.vectorMul (y, _matM, x);
					break;
#endif
					double* dx = new double[_n];
					for (size_t i=0; i<_n; i++) {
						_domain.convert(dx[i], x[i]);
					}
					if (num_chunks == 1) {
						double *ctd = new double[_m];
						cblas_dgemv(CblasRowMajor, CblasNoTrans, (int) _m, (int) _n,
							    1,  chunks, (int) _n, dx, 1, 0,  ctd, 1);

						for (size_t i=0;i<_n;++i)
							_domain.init(y[i],ctd[i]);
						delete[] ctd;
						delete[] dx;
					}
					else {
						/*
						 * rc: number of vectors to recombine
						 * (the idea is that to compute a polynomial in the base 2^chunksize
						 * with <= 53 bits in each coefficient, we can instead OR nonoverlapping blocks
						 * of bits and then add them at the end, like this:
						 *      AAAACCCCEEEEGGGG   instead  AAAA << 12 + BBBB << 10 + CCCC << 8 + ...
						 *    +   BBBBDDDDFFFF00      of
						 * also note that we need separate blocks for positive and negative entries)
						 */
						 int rc = int(52 / chunk_size) + 1; //constant at 4 for now
						 /*
						 * rclen: number of bytes in each of these OR-ed vectors
						 * needs room to hold (max long long) << (num_chunks * chunksize)
						 */
						int rclen = (int)num_chunks*2 + 5;

						unsigned char* combined = new unsigned char[(size_t)rc*_n*(size_t)rclen];
						memset(combined, 0, (size_t)rc*_n*(size_t)rclen);

						//order from major index to minor: combining index, component of sol'n, byte
						//compute a product (chunk times x) for each chunk

						double* ctd = new double[_n];
#ifdef DEBUG_CHUNK_APPLY


						std::cout<<"- A chunk --------------------------\n";
						for (size_t k=0;k<num_chunks;++k){
							for (size_t i=0;i<_m;i++){
								for (size_t j=0;j<_n;j++)
									std::cout<<integer(*(chunks+j+i*_n+k*_m*_n))<<",";
								std::cout<<std::endl;
							}
							std::cout<<"\n";
						}

						std::cout<<"- A.x chunk---------------------\n";
#endif
						for (size_t i=0; i<num_chunks; i++) {
							cblas_dgemv(CblasRowMajor, CblasNoTrans,
								    (int) _m, (int) _n, 1,
								    chunks + (_m*_n*i),(int)  _n, dx, 1, 0, ctd, 1);
#ifdef DEBUG_CHUNK_APPLY
							for (size_t j=0;j<_n;j++)
								std::cout<<integer(*(ctd+j))<<",";
							std::cout<<std::endl;
#endif

							//if (!use_neg || i<num_chunks-1)
							for (size_t j=0; j<_n; j++) {
								// up to 53 bits will be ored-in, to be summed later
								unsigned char* bitDest = combined;
								bitDest += (size_t)rclen*((i % (size_t)rc)*_n+j);
								long long mask = static_cast<long long>(ctd[j]);
								bitDest += 2*i;
								long long tmp;
								memcpy(&tmp,bitDest,sizeof(long long));
								tmp |= mask;
								memcpy(bitDest,&tmp,sizeof(long long));
							}
						}
						delete[] dx;
						for (size_t i=0; i<_n; i++) {
							LinBox::integer result, tmp;
							/*
							   if (use_neg) {
							   result = -ctd[i];
							   result <<= (num_chunks-1)*16;
							   }
							   else
							   */
							result = 0;

							for (int j=0; j<rc; j++) {
								unsigned char* thispos = combined + (size_t)rclen*((size_t)j*_n+i);
								Givaro::Protected::importWords(tmp, (size_t)rclen, -1, 1, 0, 0, thispos);
								result += tmp;
							}
							_domain.init(y[i], result);
						}
						// shift back the result
						if (use_neg) {
							Element acc;
							_domain.assign(acc,_domain.zero);
							for (size_t i=0;i<x.size();++i)
								_domain.addin(acc,x[i]);
							_domain.mulin(acc,shift);

							for (size_t i=0;i<y.size();++i)
								_domain.subin(y[i], acc);
						}
						delete[] combined;
						delete[] ctd;
					}
				}
				break;

			case VectorQadic:
				{
#ifdef TIMING_APPLY
					Timer chrono;
					chrono.clear();
					chrono.start();
#endif
					// fill vector chunks with zero
					memset(vchunks, 0, sizeof(double)*_n*num_chunks);

					// smallest distance of non-overlaping chunks results
					size_t rc = (52 / chunk_size) + 1;

					// number of bytes in a chunks
					size_t chunk_byte= (chunk_size == 32)? 4: 2;

					// number of byte to store
					size_t rclen = num_chunks*chunk_byte + 5;

					unsigned char* combined = new unsigned char[rclen];

					double *ctd= new double[_m*num_chunks];
					if (chunk_size >=32)
						create_VectorQadic_32 (_domain, x, vchunks, num_chunks);
					else
						create_VectorQadic (_domain, x, vchunks, num_chunks);
#ifdef TIMING_APPLY
					chrono.stop();
					_convert_data+=chrono;
					chrono.clear();
					chrono.start();
#endif
					cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans, (int) _m, (int) num_chunks, (int) _n, 1.,
						    chunks, (int) _n, vchunks, (int) num_chunks, 0., ctd, (int) num_chunks);

#ifdef TIMING_APPLY
					chrono.stop();
					_apply+=chrono;
					chrono.clear();
					chrono.start();
#endif

#ifdef DEBUG_CHUNK_APPLY

					std::cout<<"- A ---------------------------\n";
					for (size_t i=0;i<_m;i++){
						for (size_t j=0;j<_n;j++)
							std::cout<<integer(*(chunks+j+i*_n))<<",";
						std::cout<<std::endl;
					}


					std::cout<<"- x chunk-----------------------\n";
					for (size_t i=0;i<_n;i++){
						for (size_t j=0;j<num_chunks;j++)
							std::cout<<integer(*(vchunks+j+i*num_chunks))<<",";
						std::cout<<std::endl;
					}

					std::cout<<"- A.x chunk---------------------\n";
					for (size_t i=0;i<_m;i++){
						for (size_t j=0;j<num_chunks;j++)
							std::cout<<integer(*(ctd+j+i*num_chunks))<<",";
						std::cout<<std::endl;
					}
#endif
					for (size_t i=0; i< _m; ++i){
						integer result=0, val;
						for (size_t k=0; k< rc;++k){
							memset(combined, 0, rclen);
							unsigned char* BitDest = combined+chunk_byte*k;
							for (size_t j=k; j< num_chunks; j+=rc){
								long long mask = static_cast<long long>(ctd[i*num_chunks+j]);
								long long tmp;
								memcpy(&tmp,BitDest,sizeof(long long));
								tmp |= mask;
								memcpy(BitDest,&tmp,sizeof(long long));
								BitDest+=rc*chunk_byte;
							}
							Givaro::Protected::importWords(val, (size_t)rclen, -1, 1, 0, 0, combined);
							result+=val;
						}
						_domain.init(y[i], result);
					}

					// shift back the result
					if (use_neg) {
						Element acc;
						_domain.assign(acc,_domain.zero);
						for (size_t i=0;i<x.size();++i)
							_domain.addin(acc,x[i]);
						_domain.mulin(acc,shift);

						for (size_t i=0;i<y.size();++i)
							_domain.subin(y[i], acc);
					}

					delete[] combined;
					delete[] ctd;
#ifdef TIMING_APPLY
					chrono.stop();
					_convert_result+=chrono;
#endif

				}
				break;

			case CRT:

				{
#ifdef TIMING_APPLY
					Timer chrono;
					chrono.clear();
					chrono.start();
#endif
					size_t rns_size= _rns->size();
					integer mod, hmod;
					mod  = _rns->getCRTmodulo ();
					hmod = (mod-1)>>1;

					// fill rns vector with zero
					//memset(vchunks, 0, sizeof(double)*_n*rns_size);

					// create rns vector
					create_VectorRNS (*_rns, _domain, x, vchunks);

					// allocate memory for the result
					double *ctd= new double[_m*rns_size];
#ifdef TIMING_APPLY
					chrono.stop();
					_convert_data+=chrono;
					chrono.clear();
					chrono.start();
#endif
					// perform multiplication componentwise
					for (size_t i=0;i< rns_size; ++i)
						cblas_dgemv(CblasRowMajor, CblasNoTrans, (int) _m, (int) _n,
							    1, chunks+i*_m*_n, (int) _n, vchunks+i*_n, 1, 0, ctd+i*_m, 1);
#ifdef TIMING_APPLY
					chrono.stop();
					_apply+=chrono;
					chrono.clear();
					chrono.start();
#endif

#ifdef DEBUG_CHUNK_APPLY
					_rns->write(std::cout);
					std::cout<<"- A RNS --------------------------\n";
					for (size_t k=0;k<rns_size;++k){
						for (size_t i=0;i<_m;i++){
							for (size_t j=0;j<_n;j++)
								std::cout<<integer(*(chunks+j+i*_n+k*_n*rns_size))<<",";
							std::cout<<std::endl;
						}
						std::cout<<"\n";
					}

					std::cout<<"- x RNS-----------------------\n";
					for (size_t i=0;i<_n;i++){
						for (size_t j=0;j<rns_size;j++)
							std::cout<<integer(*(vchunks+j+i*rns_size))<<",";
						std::cout<<std::endl;
					}
					std::cout<<"- A.x RNS---------------------\n";
					for (size_t i=0;i<_m;i++){
						for (size_t j=0;j<rns_size;j++)
							std::cout<<integer(*(ctd+j+i*rns_size))<<",";
						std::cout<<std::endl;
					}
#endif


					// reconstruct the result using CRT
					std::vector<double> tmp(rns_size);
					integer res;
					for (size_t j=0;j<_m;++j){
						for (size_t i=0;i<rns_size;++i)
							_rns->getBase(i).init(tmp[i], ctd[j+i*_m]);
						_rns->convert(res, tmp);
						_domain.init(y[j], res);
						//if (y[j] > hmod) y[j]-=mod;
					}
					delete[] ctd;

#if 0
					std::cout << "y mod q: ";
					for (size_t i=0; i<y.size(); i++)
						std::cout << y[i] << ' ';
					std::cout << std::endl;
					std::cout << "b: ";
					for (size_t i=0; i<b.size(); i++)
						std::cout << b[i] << ' ';
					std::cout << std::endl;

					std::cout<<"p: "<<_prime<<" q: "<<_q<<std::endl;
#endif
					Element y_cur, b_cur;
					// finish crt according to b
					for (size_t i=0;i<_m;++i){
						_domain.mod(b_cur, b[i], _prime);
						_domain.sub(y_cur, b_cur, y[i]);//std::cout<<"(b-y): "<<y_cur<<std::endl;
						_domain.mulin(y_cur, _inv_q);//std::cout<<"((b-y)/q): "<<y_cur<<std::endl;
						_domain.modin(y_cur, _prime);//std::cout<<"((b-y)/q mod p): "<<y_cur<<std::endl;
						_domain.axpyin(y[i],_q, y_cur);//std::cout<<"y+p((b-y)/q mod p): "<<y[i]<<std::endl;
						if ( y[i] > _h_pq) y[i]-=_pq;
					}
					/*
					   std::cout << "y: ";
					   for (size_t i=0; i<y.size(); i++)
					   std::cout << y[i] << ' ';
					   std::cout << std::endl;
					   */

#ifdef TIMING_APPLY
					chrono.stop();
					_convert_result+=chrono;
#endif
				}
				break;
			}

#ifdef DEBUG_CHUNK_APPLY
			std::cout << "y: ";
			for (size_t i=0; i<y.size(); i++)
				std::cout << y[i] << ' ';
			std::cout << std::endl;
#endif


			return y;
		}


		Vector& applyVTrans(Vector& y, Vector& x) const
		{
			TransposeMatrix<IMatrix> B(_matM);
			return _MD.vectorMul (y, B, x);
		}


		IMatrix& applyM (IMatrix &Y, const IMatrix &X) const
		{

			linbox_check( _n == X.rowdim());
			linbox_check( _m == Y.rowdim());
			linbox_check( Y.coldim() == X.coldim());

			if (!use_chunks){
				_MD.mul (Y, _matM, X);
			}
			else{
				size_t _k= X.coldim();
				double* dX = new double[_n*_k];
				for (size_t i=0; i<_n; i++)
					for(size_t j=0;j<_k;++j)
						_domain.convert(dX[i*_k+j], X.getEntry(i,j));

#ifdef DEBUG_CHUNK_APPLYM
				cout << "X: ";
				X.write(cout,_domain);
				cout << endl;
#endif


				if (num_chunks == 1) {
					double *ctd = new double[_m*_k];
					cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
						    (int) _m,(int) _k,(int) _n, 1,
						    chunks,(int) _n, dX, (int) _k, 0, ctd, (int) _k);

					for (size_t i=0;i<_m;++i)
						for (size_t j=0;j<_k;++j)
							_domain.init(Y.refEntry(i,j),ctd[i*_k+j]);
					delete[] ctd;
					delete[] dX;
				}
				else {
					/*
					 * rc: number of vectors to recombine
					 *(the idea is that to compute a polynomial in the base 2^chunksize
					 * with <= 53 bits in each coefficient, we can instead OR nonoverlapping blocks
					 * of bits and then add them at the end, like this:
					 *     AAAACCCCEEEEGGGG   instead  AAAA << 12 + BBBB << 10 + CCCC << 8 + ...
					 *   +   BBBBDDDDFFFF00      of
					 * also note that we need separate blocks for positive and negative entries)
					 */

					int rc = int(52 / chunk_size) + 1; //constant at 4 for now

					//rclen: number of bytes in each of these OR-ed vectors
					// needs room to hold (max long long) << (num_chunks * chunksize)

					int rclen = (int)num_chunks*2 + 5;

					// cout << "rc= " << rc << ", rclen = " << rclen << endl;

					unsigned char* combined = new unsigned char[(size_t)rc*_m*_k*(size_t)rclen];
					memset(combined, 0, (size_t)rc*_m*_k*(size_t)rclen);

					//order from major index to minor: combining index, component of sol'n, byte

					//compute a product (chunk times x) for each chunk
					double* ctd = new double[_m*_k];

					for (size_t i=0; i<num_chunks; i++) {//!@bug why not use FFLAS ?
						cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasNoTrans,
							    (int) _m,(int) _k,(int) _n, 1,
							    chunks+(_m*_n*i),(int) _n, dX, (int) _k, 0, ctd, (int) _k);

						if (!use_neg || i<num_chunks-1)
							for (size_t j=0; j<_m*_k; j++) {
								// up to 53 bits will be ored-in, to be summed later
								unsigned char* bitDest = combined;
								bitDest += (unsigned char)rclen*((i % (size_t)rc)*_m*_k+j);
								long long mask = static_cast<long long>(ctd[j]);
								bitDest += 2*i;
								long long tmp;
								memcpy(&tmp,bitDest,sizeof(long long));
								tmp |= mask;
								memcpy(bitDest,&tmp,sizeof(long long));
							}
					}

					delete[] dX;

					for (size_t i=0; i<_m*_k; i++) {
						LinBox::integer result, tmp;
						if (use_neg) {
							result = -ctd[i];
							result <<= uint64_t((num_chunks-1)*16);
#ifdef DEBUG_CHUNK_APPLYM
							cout << "rcneg: " << result << endl;
#endif
						}
						else
							result = 0;

						for (int j=0; j<rc; j++) {
							unsigned char* thispos = combined + (unsigned char) rclen*((size_t)j*_m*_k+i);
							Givaro::Protected::importWords(tmp, (size_t)rclen, -1, 1, 0, 0, thispos);
							result += tmp;
#ifdef DEBUG_CHUNK_APPLYM
							cout << "rc[" << j << "," << i << "]:" << tmp << endl;
#endif
						}
#ifdef DEBUG_CHUNK_APPLYM
						cout << "v2[" << i << "]:" << result  << endl;
#endif

						_domain.init(*(Y.getPointer()+i), result);
					}
					delete[] combined;
					delete[] ctd;
				}
			}
			return Y;
		}



	protected:
		Domain                             _domain;
		const IMatrix                     &_matM;
		MatrixDomain<Domain>              _MD;
		size_t                             _m;
		size_t                             _n;

		// data initialize by setup
		bool           use_chunks;
		bool              use_neg;
		size_t         chunk_size;
		size_t         num_chunks;
		double *           chunks;
		double *          vchunks;
		integer             shift;
		ApplyChoice     _switcher;
		MultiModDouble      *_rns;
		Element            _prime, _q, _inv_q, _pq, _h_pq;
		mutable Timer              _apply, _convert_data, _convert_result;




	};

// #if !defined (__INTEL_COMPILER) && !defined(__clang__)
// template<>
// #endif
	template <class Domain>
	class MatrixApplyDomain<Domain, BlasMatrix<Domain> > : public BlasMatrixApplyDomain<Domain, BlasMatrix<Domain> > {

	public:
		MatrixApplyDomain (const Domain &D, const  BlasMatrix<Domain> &Mat) :
			BlasMatrixApplyDomain<Domain, BlasMatrix<Domain> > (D,Mat)
		{}

	};

#if 0
// #ifndef __INTEL_COMPILER
	// template<>
// #endif
	template <class Domain>
	class MatrixApplyDomain<Domain, BlasMatrix<Domain> > :
	public BlasMatrixApplyDomain<Domain, BlasMatrix<Domain> > {

	public:
		MatrixApplyDomain (const Domain &D, const  BlasMatrix<Domain> &Mat) :
			BlasMatrixApplyDomain<Domain, BlasMatrix<Domain> > (D,Mat)
		{}

	};
#endif



	/** \brief split an integer matrix into a padic chunk representation
	 *
	 */
	template <class Domain, class IMatrix>
	void create_MatrixQadic (const Domain           &D,
				 const IMatrix          &Mat,
				 double            *chunks,
				 size_t         num_chunks,
				 const integer    shift)
	{

		typename IMatrix::ConstIterator it= Mat.Begin();

		size_t m,n,mn;
		m  = Mat.rowdim();
		n  = Mat.coldim();
		mn = m*n;



		if (num_chunks ==1)
			for (size_t i=0; i<mn; ++i, ++it)
				D.convert(*(chunks+i), *it);
		else {
			for (size_t i=0; i<mn; ++i, ++it) {
				size_t tmpsize, tmpbitsize, j;
				integer tmp;
				double* pdbl = chunks + i;
				D.convert(tmp, *it);
				tmp=tmp+shift;
				tmpsize    = tmp.size();
				tmpbitsize = tmp.bitsize();

				if (tmp ==0) {
					*pdbl=0;
				}
				else
					if (tmp > 0) {

						//if (sizeof(long)==8 ) {
#if __SIZEOF_LONG__  == 8

						// specialization for 64bits integer limbs
						for (j=0; j<tmpsize-1; j++) {
							*pdbl        = double( tmp[j]        & 0xFFFF);
							*(pdbl+mn)   = double((tmp[j] >> 16) & 0xFFFF);
							*(pdbl+2*mn) = double((tmp[j] >> 32) & 0xFFFF);
							*(pdbl+3*mn) = double((tmp[j] >> 48) & 0xFFFF);
							pdbl      += 4*mn;
						}
						if ((tmpbitsize - j*64) > 0 ) {
							*pdbl = double(tmp[tmpsize-1]&0xFFFF);
							pdbl+=mn;
						}
						if ((tmpbitsize - j*64) > 16 ) {
							*pdbl = double((tmp[tmpsize-1] >> 16)& 0xFFFF);
							pdbl+=mn;
						}
						if ((tmpbitsize - j*64) > 32 ) {
							*pdbl = double((tmp[tmpsize-1] >> 32)& 0xFFFF);
							pdbl+=mn;
						}
						if ((tmpbitsize - j*64) > 48 )
							*pdbl = double((tmp[tmpsize-1] >> 48)& 0xFFFF);
						//}
						// else {
#else
						// specialization for 32bits integer limbs
						for (j=0; j<tmpsize-1; j++) {
							*pdbl      = double(tmp[j] &  0xFFFF);
							*(pdbl+mn) = tmp[j] >> 16;
							pdbl      += 2*mn;
						}
						if ((tmpbitsize - j*32) > 16 ) {
							*pdbl      = double(tmp[tmpsize-1] &  0xFFFF);
							*(pdbl+mn) = tmp[tmpsize-1] >> 16;
						}
						else {
							*pdbl      = double(tmp[tmpsize-1] & 0xFFFF);
						}
						//}
#endif
					}
					else {
						++tmp;
#if __SIZEOF_LONG__ == 8
						// specialization for 64bits integer limbs
						for (j=0; j<tmpsize-1; j++) {
							*pdbl        = double(0xFFFF ^ ( tmp[j]        & 0xFFFF));
							*(pdbl+mn)   = double(0xFFFF ^ ((tmp[j] >> 16) & 0xFFFF));
							*(pdbl+2*mn) = double(0xFFFF ^ ((tmp[j] >> 32) & 0xFFFF));
							*(pdbl+3*mn) = double(0xFFFF ^ ((tmp[j] >> 48) & 0xFFFF));
							pdbl        += 4*mn;
						}

						j=j<<2;
						if ((tmpbitsize - j*64) > 0 ) {
							*pdbl = double(0xFFFF ^ (tmp[tmpsize-1]&0xFFFF));
							pdbl+=mn;
							++j;
						}
						if ((tmpbitsize - j*64) > 16 ) {
							*pdbl = double(0xFFFF ^ ((tmp[tmpsize-1] >> 16)& 0xFFFF));
							pdbl+=mn;
							++j;
						}
						if ((tmpbitsize - j*64) > 32 ) {
							*pdbl = double(0xFFFF ^ ((tmp[tmpsize-1] >> 32)& 0xFFFF));
							pdbl+=mn;
							++j;
						}
						if ((tmpbitsize - j*64) > 48 ) {
							*pdbl = double(0xFFFF ^ ((tmp[tmpsize-1] >> 48)& 0xFFFF));
							++j;
						}

						for (; j<num_chunks-1; j++, pdbl += mn)
							*pdbl      = double(0xFFFF);
						*pdbl = 1;
#else
						// specialization for 32bits integer limb
						for (j=0; j<tmpsize-1; j++) {
							*pdbl      = double(0xFFFF ^ (tmp[j] & 0xFFFF));
							*(pdbl+mn) = double(0xFFFF ^ (tmp[j] >> 16));
							pdbl      += 2*mn;
						}
						j=j<<1;
						if ((tmpbitsize -j*32) > 16) {
							*pdbl      = double(0xFFFF ^ (tmp[tmpsize-1] & 0xFFFF));
							*(pdbl+mn) = double(0xFFFF ^ (tmp[tmpsize-1] >> 16));
							pdbl      += 2*mn;
							j+=2;
						}
						else {
							*pdbl      = double(0xFFFF ^ (tmp[tmpsize-1] & 0xFFFF));
							pdbl      += mn;
							j+=1;
						}

						for (; j<num_chunks-1; j++, pdbl += mn)
							*pdbl      = double(0xFFFF);
						*pdbl = 1;
#endif
					}
			}
		}
	}

	/** \brief split an integer vector into a padic chunk representation
	 *
	 */
	template <class Domain, class Vector>
	void create_VectorQadic (const Domain            &D,
				 const Vector            &V,
				 double             *chunks,
				 size_t          num_chunks)
	{


		typename Vector::const_iterator it= V.begin();

		size_t mn;
		mn = V.size();


		if (num_chunks ==1)
			for (size_t i=0; i<mn; ++i, ++it)
				D.convert(*(chunks+i), *it);
		else {
			for (size_t i=0; i<mn; ++i, ++it) {
				size_t tmpsize, tmpbitsize, j;
				integer tmp;
				double* pdbl = chunks + i*num_chunks;
				D.convert(tmp, *it);
				tmpsize    = tmp.size();
				tmpbitsize = tmp.bitsize();

				if (tmp ==0)
					*pdbl=0;
				else
					if (tmp > 0) {


#if __SIZEOF_LONG__ == 8
						// specialization for 64bits integer limbs
						for (j=0; j<tmpsize-1; j++) {
							*pdbl     = double( tmp[j]        & 0xFFFF);
							*(pdbl+1) = double((tmp[j] >> 16) & 0xFFFF);
							*(pdbl+2) = double((tmp[j] >> 32) & 0xFFFF);
							*(pdbl+3) = double((tmp[j] >> 48) & 0xFFFF);
							pdbl     += 4;
						}
						if ((tmpbitsize - j*64) > 0 ) {
							*pdbl = double(tmp[tmpsize-1]&0xFFFF);
							pdbl++;
						}
						if ((tmpbitsize - j*64) > 16 ) {
							*pdbl = double((tmp[tmpsize-1] >> 16)& 0xFFFF);
							pdbl++;
						}
						if ((tmpbitsize - j*64) > 32 ) {
							*pdbl = double((tmp[tmpsize-1] >> 32)& 0xFFFF);
							pdbl++;
						}
						if ((tmpbitsize - j*64) > 48 )
							*pdbl = double((tmp[tmpsize-1] >> 48)& 0xFFFF);
#else
						// specialization for 32bits integer limbs
						for (j=0; j<tmpsize-1; j++) {
							*pdbl     = double(tmp[j] &  0xFFFF);
							*(pdbl+1) = tmp[j] >> 16;
							pdbl     += 2;
						}
						if ((tmpbitsize - j*32) > 16 ) {
							*pdbl     = double(tmp[tmpsize-1] &  0xFFFF);
							*(pdbl+1) = tmp[tmpsize-1] >> 16;
						}
						else {
							*pdbl      = double(tmp[tmpsize-1] & 0xFFFF);
						}
#endif
					}
					else {
						++tmp;
#if __SIZEOF_LONG__ == 8
						// specialization for 64bits integer limbs
						for (j=0; j<tmpsize-1; j++) {
							*pdbl     = (double) (0xFFFF ^ ( tmp[j]        & 0xFFFF));
							*(pdbl+1) = (double) (0xFFFF ^ ((tmp[j] >> 16) & 0xFFFF));
							*(pdbl+2) = (double) (0xFFFF ^ ((tmp[j] >> 32) & 0xFFFF));
							*(pdbl+3) = (double) (0xFFFF ^ ((tmp[j] >> 48) & 0xFFFF));
							pdbl     += 4   ;
						}

						j=j<<2;
						if ((tmpbitsize - j*64) > 0 ) {
							*pdbl = (double) (0xFFFF ^ (tmp[tmpsize-1]&0xFFFF));
							pdbl++;
							++j;
						}
						if ((tmpbitsize - j*64) > 16 ) {
							*pdbl = (double) (0xFFFF ^ ((tmp[tmpsize-1] >> 16)& 0xFFFF));
							pdbl++;
							++j;
						}
						if ((tmpbitsize - j*64) > 32 ) {
							*pdbl = (double) (0xFFFF ^ ((tmp[tmpsize-1] >> 32)& 0xFFFF));
							pdbl++;
							++j;
						}
						if ((tmpbitsize - j*64) > 48 ) {
							*pdbl = (double) (0xFFFF ^ ((tmp[tmpsize-1] >> 48)& 0xFFFF));
							++j;
						}

						for (; j<num_chunks-1; j++, pdbl += mn)
							*pdbl      = (double) (0xFFFF);
						*pdbl = 1;
#else
						// specialization for 32bits integer limb
						for (j=0; j<tmpsize-1; j++) {
							*pdbl      = (double) (0xFFFF ^ (tmp[j] & 0xFFFF));
							*(pdbl+1)  = (double) (0xFFFF ^ (tmp[j] >> 16));
							pdbl      += 2;
						}
						j=j<<1;
						if ((tmpbitsize -j*32) > 16) {
							*pdbl      = (double) (0xFFFF ^ (tmp[tmpsize-1] & 0xFFFF));
							*(pdbl+1)  = (double) (0xFFFF ^ (tmp[tmpsize-1] >> 16));
							pdbl      += 2;
							j+=2;
						}
						else {
							*pdbl      = (double) (0xFFFF ^ (tmp[tmpsize-1] & 0xFFFF));
							pdbl      += 1;
							j+=1;
						}

						for (; j<num_chunks-1; j++, pdbl += mn)
							*pdbl      = (double) (0xFFFF);
						*pdbl = 1;
#endif
					}
			}
	}

	}

	/** \brief split an integer vector into a padic chunk representation
	 *
	 */
	template <class Domain, class Vector>
	void create_VectorQadic_32 (const Domain            &D,
				    const Vector            &V,
				    double             *chunks,
				    size_t          num_chunks)
	{


		typename Vector::const_iterator it= V.begin();

		size_t mn;
		mn = V.size();


		if (num_chunks ==1)
			for (size_t i=0; i<mn; ++i, ++it)
				D.convert(*(chunks+i), *it);
		else {
			for (size_t i=0; i<mn; ++i, ++it) {
		size_t tmpsize, /*tmpbitsize,*/ j;
				integer tmp;
				double* pdbl = chunks + i*num_chunks;
				D.convert(tmp, *it);
				tmpsize    = tmp.size();
				//tmpbitsize = tmp.bitsize();

				if (tmp ==0)
					*pdbl=0;
				else
					if (tmp > 0) {


#if __SIZEOF_LONG__ == 8
						// specialization for 64bits integer limbs
						size_t tmpbitsize = tmp.bitsize();
						for (j=0; j<tmpsize-1; j++) {
							*pdbl     =  double(tmp[j] & 0xFFFFFFFF);
							*(pdbl+1) =  double(tmp[j] >> 32);
							pdbl     += 2;
						}
						if ((tmpbitsize - j*64) > 0 ) {
							*pdbl = double(tmp[tmpsize-1]&0xFFFFFFFF);
							pdbl++;
						}
#else
						// specialization for 32bits integer limbs
						for (j=0; j<tmpsize; j++) {
							*pdbl     = tmp[j] ;
							pdbl     += 1;
						}
#endif
					}
					else {
						++tmp;
#if __SIZEOF_LONG__ == 8
						// specialization for 64bits integer limbs
						size_t tmpbitsize = tmp.bitsize();
						for (j=0; j<tmpsize; j++) {
							*pdbl     = double(0xFFFFFFFF ^ ( tmp[j] & 0xFFFFFFFF));
							*(pdbl+2) = double(0xFFFFFFFF ^ ( tmp[j] >> 32));
							pdbl     += 2   ;
						}

						j=j<<2;
						if ((tmpbitsize - j*64) > 0 ) {
							*pdbl = double(0xFFFFFFFF ^ (tmp[tmpsize-1]&0xFFFFFFFF));
							pdbl++;
							++j;
						}

						if ((tmpbitsize - j*64) > 32 ) {
							*pdbl = double(0xFFFF ^ (tmp[tmpsize-1] >> 32));
							pdbl++;
							++j;
						}

						for (; j<num_chunks-1; j++, pdbl += mn)
							*pdbl      = double(0xFFFFFFFF);
						*pdbl = 1;
#else
						// specialization for 32bits integer limb
						for (j=0; j<tmpsize-1; j++) {
							*pdbl      = double(0xFFFFFFFF ^ tmp[j]);
							pdbl      += 1;
						}
						j=j<<1;

						for (; j<num_chunks-1; j++, pdbl += mn)
							*pdbl      = double(0xFFFF);
						*pdbl = 1;
#endif

					}
			}
	}

	}


	template <class Domain, class IMatrix>
	void create_MatrixRNS (const MultiModDouble    &F,
			       const Domain            &D,
			       const IMatrix           &Mat,
			       double             *chunks)
	{


		size_t rns_size= F.size();
		typename IMatrix::ConstIterator it = Mat.Begin();
		size_t mn = Mat.rowdim()*Mat.coldim();
		integer tmp;

		for (size_t i=0; i< mn; ++i, ++it){
			D.convert(tmp,*it);
			for (size_t j=0;j< rns_size; ++j)
				F.getBase(j).init(chunks[i+j*mn], tmp);
		}
	}


	template <class Domain, class IVector>
	void create_VectorRNS (const MultiModDouble    &F,
			       const Domain            &D,
			       const IVector           &V,
			       double             *chunks)
	{

		size_t rns_size= F.size();
		typename IVector::const_iterator it= V.begin();
		size_t mn = V.size();
		integer tmp;

		for (size_t i=0; i< mn; ++i, ++it){
			D.convert(tmp, *it);
			for (size_t j=0;j< rns_size; ++j)
				F.getBase(j).init(chunks[i+j*mn], tmp);
		}
	}


} // end of namespace LinBox

#endif // __LINBOX_apply_H

// Local Variables:
// mode: C++
// tab-width: 4
// indent-tabs-mode: nil
// c-basic-offset: 4
// End:
// vim:sts=4:sw=4:ts=4:et:sr:cino=>s,f0,{0,g0,(0,\:0,t0,+0,=s
