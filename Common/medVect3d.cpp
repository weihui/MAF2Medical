/*=========================================================================
Program:   Multimod Application Framework
Module:    $RCSfile: medLogicWithManagers.cpp,v $
Language:  C++
Date:      $Date: 2011-06-16 09:12:22 $
Version:   $Revision: 1.4.2.2 $
Authors:   Gianluigi Crimi
==========================================================================
Copyright (c) 2002/2004
CINECA - Interuniversity Consortium (www.cineca.it) 
SCS s.r.l. - BioComputing Competence Centre (www.scsolutions.it - www.b3c.it)

MafMedical Library use license agreement

The software named MafMedical Library and any accompanying documentation, 
manuals or data (hereafter collectively "SOFTWARE") is property of the SCS s.r.l.
This is an open-source copyright as follows:
Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:
* Redistributions of source code must retain the above copyright notice, 
this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation and/or 
other materials provided with the distribution.
* Modified source versions must be plainly marked as such, and must not be misrepresented 
as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 'AS IS' 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND 
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

MafMedical is partially based on OpenMAF.
=========================================================================*/

#include "medDefines.h" 
//----------------------------------------------------------------------------
// NOTE: Every CPP file in the MAF must include "mafDefines.h" as first.
// This force to include Window,wxWidgets and VTK exactly in this order.
// Failing in doing this will result in a run-time error saying:
// "Failure#0: The value of ESP was not properly saved across a function call"
//----------------------------------------------------------------------------

#include "medDecl.h"
#include "medVect3d.h"
#include <math.h>


//----------------------------------------------------------------------------
medVect3d medVect3d::Abs(void)
//----------------------------------------------------------------------------
{
  //Generate a new vector whit the absolute
  //value of each original element
  return medVect3d (fabsf(m_X),fabsf(m_Y),fabsf(m_Z));
}

//----------------------------------------------------------------------------
medVect3d::medVect3d()
//----------------------------------------------------------------------------
{
  //sets each element to zero for default constructor
  m_X=m_Y=m_Z=0.0;
}

//----------------------------------------------------------------------------
medVect3d::medVect3d(double x, double y, double z)
//----------------------------------------------------------------------------
{
  //specified constructor, sets each element to the specified value
  m_X = x; m_Y = y; m_Z = z;
}

//----------------------------------------------------------------------------
medVect3d::medVect3d( double *values )
//----------------------------------------------------------------------------
{
  //specified constructor, sets each element to the specified value
  //from the input array
   m_X = values[0]; m_Y = values[1]; m_Z = values[2];
}

//----------------------------------------------------------------------------
void medVect3d::Setzero(void)
//----------------------------------------------------------------------------
{
  //sets each element to zero
  m_X=m_Y=m_Z=0.0;
}

//----------------------------------------------------------------------------
int medVect3d::operator==(medVect3d vect)
//----------------------------------------------------------------------------
{
  //return true if each element is equal to each other pair
  return ( (vect.m_X==m_X) && (vect.m_Y==m_Y) && (vect.m_Z==m_Z) );
}

//----------------------------------------------------------------------------
medVect3d medVect3d::operator+(medVect3d vect)
//----------------------------------------------------------------------------
{
  //Generating a new vector witch the sum of each element 
  return medVect3d(vect.m_X + m_X, vect.m_Y + m_Y, vect.m_Z + m_Z);
}

//----------------------------------------------------------------------------
medVect3d medVect3d::operator+=(medVect3d vect)
//----------------------------------------------------------------------------
{
  //assign at each value the sum whit the correspondent element
  m_X+=vect.m_X;
  m_Y+=vect.m_Y;
  m_Z+=vect.m_Z;
  //Return a pointer to this object (for concatenating)
  //i.e. a+=(b+=c)
  return *this;
}

//----------------------------------------------------------------------------
medVect3d medVect3d::operator-=(medVect3d vect)
//----------------------------------------------------------------------------
{
  //assign at each value the difference whit the correspondent element
  m_X-=vect.m_X;
  m_Y-=vect.m_Y;
  m_Z-=vect.m_Z;
  //Return a pointer to this object (for concatenating)
  //i.e. a-=(b-=c)
  return *this;
}

//----------------------------------------------------------------------------
medVect3d medVect3d::operator*=(double val)
  //----------------------------------------------------------------------------
{
  //assign at each value the product whit the correspondent element
  m_X*=val;
  m_Y*=val;
  m_Z*=val;
  //Return a pointer to this object (for concatenating)
  //i.e. a*=(b*=c)
  return *this;
}
//----------------------------------------------------------------------------
medVect3d medVect3d::operator/=(double val)
//----------------------------------------------------------------------------
{
  val=1.0/val;
  //assign at each value the sum whit the correspondent element
  m_X*=val;
  m_Y*=val;
  m_Z*=val;
  //Return a pointer to this object (for concatenating)
  //i.e. a+=(b+=c)
  return *this;
}

//----------------------------------------------------------------------------
medVect3d medVect3d::operator-(medVect3d vect)
//----------------------------------------------------------------------------
{
  //Generating a new vector witch the difference of each element 
  return medVect3d(m_X - vect.m_X, m_Y - vect.m_Y, m_Z - vect.m_Z);
}

//----------------------------------------------------------------------------
medVect3d medVect3d::operator*(double num)
//----------------------------------------------------------------------------
{
  //Generating a new vector witch the multiplication of each element 
  return medVect3d(m_X * num, m_Y * num, m_Z * num);
}


//----------------------------------------------------------------------------
medVect3d medVect3d::operator/(double num)
//----------------------------------------------------------------------------
{
  //Generating a new vector witch the division of each element     
  num=1.0/num;
  return medVect3d(m_X * num, m_Y * num, m_Z * num);
}


//----------------------------------------------------------------------------
medVect3d medVect3d::Cross(medVect3d vector)
//----------------------------------------------------------------------------
{
  //Generating a new vector witch the cross product of the vectors
  return medVect3d(m_Y * vector.m_Z - m_Z * vector.m_Y, m_Z * vector.m_X - m_X * vector.m_Z, m_X * vector.m_Y - m_Y * vector.m_X);
}


//----------------------------------------------------------------------------
double medVect3d::Magnitude(void)
//----------------------------------------------------------------------------
{
  //calculating the magnitude of the vector
  return (double)sqrt( (m_X *m_X) + (m_Y * m_Y) + (m_Z * m_Z) );
}


//----------------------------------------------------------------------------
void medVect3d::Normalize(void)
//----------------------------------------------------------------------------
{
  //normalize this vector
  double magnitude = Magnitude();
  //Avoid division by zero
  if (magnitude != 0)
  {
    // Divide the X value of our normal by it's magnitude
    m_X /= magnitude;	
    // Divide the Y value of our normal by it's magnitude
    m_Y /= magnitude;
    // Divide the Z value of our normal by it's magnitude
    m_Z /= magnitude;
  }
}

//----------------------------------------------------------------------------
medVect3d medVect3d::Normal(void)
//----------------------------------------------------------------------------
{
  //Generating a new vector witch the normalized values of each element   
  double magnitude = Magnitude();
  //Avoid division by zero
  if (magnitude != 0)
  {
    // Divide the X, Y, Z values of our vector by it's magnitude
    return medVect3d(m_X/magnitude,m_Y/magnitude,m_Z/magnitude); 
  }
  else return medVect3d(0.0,0.0,0.0);
}

//----------------------------------------------------------------------------
double medVect3d::Dot(medVect3d vector)
//----------------------------------------------------------------------------
{
  //returns the dot product of the vector
  return m_X * vector.m_X + m_Y * vector.m_Y + m_Z * vector.m_Z;
}


//----------------------------------------------------------------------------
double medVect3d::Distance(medVect3d vect)
//----------------------------------------------------------------------------
{
  //calculating the distance between two vector
  return sqrt( ((m_X-vect.m_X) * (m_X-vect.m_X)) +
    ((m_Y-vect.m_Y) * (m_Y-vect.m_Y)) +
    ((m_Z-vect.m_Z) * (m_Z-vect.m_Z)) );
}

//----------------------------------------------------------------------------
double medVect3d::Distance2(medVect3d vect)
//----------------------------------------------------------------------------
{
  //calculating the quadratic distance between two vector
  return ((m_X-vect.m_X) * (m_X-vect.m_X)) +
    ((m_Y-vect.m_Y) * (m_Y-vect.m_Y)) +
    ((m_Z-vect.m_Z) * (m_Z-vect.m_Z)) ;
}

//----------------------------------------------------------------------------
void medVect3d::SetValues( double *values )
//----------------------------------------------------------------------------
{
  //update the values
  m_X = values[0]; m_Y = values[1]; m_Z = values[2];
}

//----------------------------------------------------------------------------
void medVect3d::SetValues( double x, double y, double z )
//----------------------------------------------------------------------------
{
  //update the values
  m_X = x; m_Y = y; m_Z = z;
}

//-----------------------------------------------------------------------
double medVect3d::AngleBetweenVectors( medVect3d vect )
  //-----------------------------------------------------------------------
{
  // Get the dot product of the vectors
  double dotProduct = this->Dot(vect);

  // Get the product of both of the vectors magnitudes
  double vectorsMagnitude = this->Magnitude() * vect.Magnitude() ;

  // Get the angle in radians between the 2 vectors
  double angle = acos( dotProduct / vectorsMagnitude );


  // Here we make sure that the angle is not a -1.#IND0000000 number, which means indefinite
  if(_isnan(angle))
    return 0;

  // Return the angle in radians
  return( angle );
}