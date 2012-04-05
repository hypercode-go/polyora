/*  This file is part of Polyora, a multi-target tracking library.
    Copyright (C) 2010 Julien Pilet

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program. If not, see <http://www.gnu.org/licenses/>.

    To contact the author of this program, please send an e-mail to:
    julien.pilet(at)calodox.org
*/
/* Julien Pilet, 2009. */
#include <limits>
#include "patchtagger.h"
#include <iostream>
#include <math.h>
#include "kmeantree.h"

#ifndef M_2PI
#define M_2PI 6.283185307179586476925286766559f
#endif
#ifndef M_PI
#define M_PI 3.141592653589793238462643383279f
#endif
#ifdef WIN32
static inline double drand48() {
	return (double)rand()/(double)RAND_MAX;
}
// win32 compatibility
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#endif

float patch_tagger::tot_weight=0;
float patch_tagger::_weight[patch_tagger::patch_size][patch_tagger::patch_size];

patch_tagger::descriptor::descriptor()
{
	total=0;
	cvInitMatHeader(&rotated, patch_size,patch_size, CV_32FC1, _rotated);
}

patch_tagger::descriptor::descriptor(const patch_tagger::descriptor &a) 
{
	*this = a;
}

const patch_tagger::descriptor &patch_tagger::descriptor::operator=(const patch_tagger::descriptor &a)
{
	memcpy(this, &a, sizeof(patch_tagger::descriptor));
	cvInitMatHeader(&rotated, patch_size,patch_size, CV_32FC1, _rotated);
	return *this;
}

float patch_tagger::descriptor::correl(const patch_tagger::descriptor &a)
{
	const float *pa = &_rotated[0][0];
	const float *pb = &a._rotated[0][0];
	float r=0;
	float norma = 0;
	float normb = 0;
	for (unsigned n = patch_size*patch_size; n; --n) {
		float va = (*pa++ - .5f);
		float vb = (*pb++ - .5f);

		norma += va*va;
		normb += vb*vb;
		r += va*vb;
	}
	//return r / (tot_weight * tot_weight);
	//std::cout << "norma: " << norma << ", normb: " << normb << ", totw: " << tot_weight << ", r: " << r << std::endl;
	return r / (sqrt(norma)*sqrt(normb));
}

static unsigned rand_range(unsigned max) {

	return (unsigned)(drand48()*max);
}

patch_tagger *patch_tagger::singleton() {
	static patch_tagger *t=0;
	if (!t) {
		t = new patch_tagger();
		t->precalc();
	}
	return t;
}

void patch_tagger::precalc() {
	for (int a=-255; a<256; a++)
		for (int b=-255;b<256;b++) {
			double angle = atan2((double)b,(double)a)/(2*M_PI);
			if (angle<0) angle+=1;
			if (angle>=1) angle-=1;
			unsigned l = (unsigned)floor(256*sqrt((double)(a*a+b*b)));
			float obinf = nb_orient * angle +.5;
			unsigned o1 = (unsigned)floor(obinf);
			float r = obinf - o1;
			unsigned o2;
			unsigned l1,l2;
			if (r>0.5f) {
				o2 = (o1+1) % nb_orient;
				r -= .5f;
				l1 = (unsigned)(l * (1.0f-r));
				l2 = (unsigned)(l * r);
			} else {
				o2 = (o1-1+nb_orient) % nb_orient;
				l1 = (unsigned)(l * (r+.5));
				l2 = (unsigned)(l * (.5f-r));
			}
			//if (l1>255) std::cerr << l1 << " should be <255!\n";
			//if (l2>255) std::cerr << l2 << " should be <255!\n";

			cart2polar_table[255+b][255+a].dir1 = o1;
			cart2polar_table[255+b][255+a].length1 = l1;
			cart2polar_table[255+b][255+a].dir2 = o2;
			cart2polar_table[255+b][255+a].length2 = l2;
		}

	float c= patch_size/2.0f -1 ;

	cvInitMatHeader(&weight, patch_size, patch_size, CV_32FC1, _weight);
	cvInitMatHeader(&mask, patch_size, patch_size, CV_8UC1, _mask);

	unsigned char patch_im[patch_size][patch_size][3];

	tot_weight = 0;
	for (unsigned y=0; y<patch_size; y++) {

		for (unsigned x=0; x<patch_size; x++) {
			float dx = x-c;
			float dy = y-c;
			
			float dc2 = 4*(dx*dx+dy*dy)/(patch_size*patch_size);
			float wdc = exp(-dc2*dc2*1.8);
			_weight[y][x] = wdc;
			tot_weight += wdc;
			_mask[y][x] = (wdc>.8 ? 0xff:0);

			float angle = nb_zone * atan2(dy,dx)/(2*M_PI);
			if (angle<0) angle+=nb_zone;
			if (angle>=nb_zone) angle-=nb_zone;

			unsigned z = floor(angle);
			assert(z<nb_zone);
			float dz;
			if (angle-z > .5) 
				dz = (angle-z-0.5)/(0.5 );
			else
				dz = (z+0.5-angle)/(0.5 );


			weight_table[y][x].zone1 = z;
			weight_table[y][x].weight1 = (unsigned)( wdc* exp(-dz*dz*dz*dz*1.5) * 4096.0f);

			dz = 1-dz;
			weight_table[y][x].weight2 = (unsigned)(wdc * exp(-dz*dz*dz*dz*1.5) * 4096.0f);
			if (angle-z < .5) {
				weight_table[y][x].zone2 = (z-1+nb_zone)%nb_zone;
			}  else {
				weight_table[y][x].zone2 = (z+1+nb_zone)%nb_zone;
			}

			//unsigned w1 = weight_table[y][x].weight1*256/4096;
			unsigned w2 = weight_table[y][x].weight1*256/4096;
			unsigned z2 = weight_table[y][x].zone1;
			patch_im[y][x][0] =  w2 * (z2&1);
			patch_im[y][x][1] =  w2*((z2&1) ==0);
			patch_im[y][x][2] =  w2*((z2&2) ==0);
		}
	}
	CvMat mat;
	cvInitMatHeader(&mat, patch_size, patch_size, CV_8UC3, patch_im);
	//cvSaveImage("patch_weight.png", &mat);

	// create random projection tests
	for (unsigned i=0; i<nb_test; i++) {
		random_tests[i].zone1 = rand_range(nb_zone);
		random_tests[i].orient1 = rand_range(nb_orient);
		do {
			random_tests[i].zone2 = rand_range(nb_zone);
			random_tests[i].orient2 = rand_range(nb_orient);
		} while (random_tests[i].zone1==random_tests[i].zone2 &&
			random_tests[i].orient2==random_tests[i].orient1);
	}
}

void patch_tagger::cmp_orientation_histogram(CvMat *patch,
					     patch_tagger::descriptor *d) {
        assert(patch->rows == patch_size);
        assert(patch->cols == patch_size);
	const int h = patch_size - 1;
	const int w = patch_size - 1;
	for (int y=1; y<h; y++) {
		unsigned char *line = &CV_MAT_ELEM(*patch, unsigned char, y, 0);
		unsigned char *line_up = &CV_MAT_ELEM(*patch, unsigned char, y-1, 0);
		unsigned char *line_down = &CV_MAT_ELEM(*patch, unsigned char, y+1, 0);
		histo_entry *weight = &weight_table[y][0];

		// striped inner loop
		for (int x=1; x<w; x++) {
			int dx = 255+(line[x+1]-line[x-1])/2;
			int dy = 255+(line_down[x]-line_up[x])/2;

			grad2polar_entry &polar(cart2polar_table[dy][dx]);

                        unsigned z1 = weight[x].zone1;
                        unsigned z2 = weight[x].zone2;
                        unsigned w1 = weight[x].weight1;
                        unsigned w2 = weight[x].weight2;
                        d->histo[z1][polar.dir1] += w1 * polar.length1;
                        d->histo[z1][polar.dir2] += w1 * polar.length2;
                        d->histo[z2][polar.dir1] += w2 * polar.length1;
                        d->histo[z2][polar.dir2] += w2 * polar.length2;
		}
	}
}

void patch_tagger::cmp_orientation(CvMat *patch, patch_tagger::descriptor *d) {
	cmp_orientation_histogram(patch, d);

	unsigned max_val=0; 
	unsigned max_o=0;
	for (unsigned o=0; o<nb_orient; o++) {
		for (unsigned z=0; z<nb_zone; z++) {
			d->sum_orient[o] += d->histo[z][o];
		}
		d->total += d->sum_orient[o];

		if (d->sum_orient[o]>=max_val) {
			max_val = d->sum_orient[o];
			max_o = o;
		}
	}
	/*
	if (d->total==0) {
		cvSaveImage("buggy_patch.bmp", patch);
	}
	*/
	//assert(d->total>0);

	float y0 = d->sum_orient[(max_o-1+nb_orient)%nb_orient];
	float y1 = d->sum_orient[max_o];
	float y2 = d->sum_orient[(max_o+1)%nb_orient];

	/*
	 y = a*x2 + b*x + c 
	 with y0 = a -b +c
	      y1 = c
	      y2 = a +b +c

	   =>
	      y0+y2 = 2*a + 2*y1
	      a = (y0+y2)/2-y1
	      b = (y0-y2)/-2 
	      c = y1

	      y'= 0 = 2*a*x +b;
	      x = -b/(2*a)
	*/
	float a = (y0+y2)/2.0f-y1;
	float b = (y0-y2)/-2.0f;
	float x = -b/(2*a);

	d->orientation = (max_o+x)*M_2PI/nb_orient;
	if (d->orientation<0) d->orientation+=M_2PI;
}	

void patch_tagger::cmp_descriptor(CvMat *patch, patch_tagger::descriptor *d, float sbpix_u, float sbpix_v)
{
	memset(d,0,sizeof(descriptor));

	cmp_orientation(patch, d);

	//d->projection = project(d);
	// fetch the rotated patch
        float scale = patch->cols / (float)patch_size ;
        float ca = cos(d->orientation) * scale;
        float sa = sin(d->orientation) * scale;

	float mat[] = {
                ca, -sa, sbpix_u + patch->cols/2,
                sa, ca, sbpix_v + patch->rows/2};
	CvMat rotm;
	cvInitMatHeader(&d->rotated,patch_size,patch_size, CV_32FC1, d->_rotated);
	cvInitMatHeader(&rotm,2,3,CV_32FC1,mat);
	cvGetQuadrangleSubPix(patch, &d->rotated, &rotm);
	CvScalar mean, stdev;
	cvAvgSdv(&d->rotated, &mean, &stdev, &mask);
	if (fabs(stdev.val[0]) < 0.0001) {
		//cvSaveImage("buggy_patch.bmp", &d->rotated);
		//std::cout << "stdev=0 in patch!! saving buggy_patch.bmp\n";
		stdev.val[0]=1;
		d->total=0;
		cvSetZero(&d->rotated);
	} else {
		// f = (ai-mu)*(wi/s) + .5 =
		float s = .5/stdev.val[0];
		for (unsigned y=0;y<patch_size; y++)
			for (unsigned x=0;x<patch_size; x++)
				d->_rotated[y][x] = (d->_rotated[y][x]-mean.val[0])*_weight[y][x]*s +.5;
	}
	//cvAddS(&d->rotated,&d->rotated, cvScalarAll(-mean.val[0]));
	//cvMul(&d->rotated, &weight, &d->rotated, .5/stdev.val[0]);
	//cvAddS(&d->rotated,&d->rotated, cvScalarAll(.5));
	//cvScale(&d->rotated, &d->rotated, .5/stdev.val[0], .5-.5*mean.val[0]/stdev.val[0]);
	//cvMul(&d->rotated, &weight,&d->rotated,

}

unsigned patch_tagger::project(patch_tagger::descriptor *d) 
{
	unsigned r=0;
	unsigned mean = d->total/(nb_zone*nb_orient);
	for (unsigned i=0; i<nb_test; i++) {
		int oz = d->orientation*(nb_zone/M_2PI);
		int o = d->orientation*(nb_orient/M_2PI);
		unsigned a = d->histo[(oz + random_tests[i].zone1) %nb_zone]
			[(o+ random_tests[i].orient1) % nb_orient];
		//unsigned b = d->histo[(oz + random_tests[i].zone2)%nb_zone]
		//	[(o+ random_tests[i].orient2) % nb_orient];
		//if (a>b*2) r += 1<<i;
		if (a>mean) r += 1<<i;
	}
	
	return r;
}

void patch_tagger::descriptor::array(float *array)
{
#ifdef WITH_PATCH_AS_DESCRIPTOR
	assert(patch_size*patch_size == kmean_tree::descriptor_size);
	memcpy(array, _rotated, patch_size*patch_size*sizeof(float));
#else
	assert(kmean_tree::descriptor_size == 128);
	cv::Mat _rot(&rotated);
	compute_sift_descriptor(array, _rot);
#endif
	/*
	float *p = array;
	for (unsigned z=0;z<nb_zone;z++) {
		for (unsigned i=0; i<nb_orient; i++) {
			int oz = orientation*(nb_zone/M_2PI);
			int o = orientation*(nb_orient/M_2PI);
			unsigned a = histo[(oz + z) %nb_zone] [(o+i) % nb_orient];
			*p++ = ((float)a/total);
		}
	}
	*/
}

