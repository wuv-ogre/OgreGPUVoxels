#ifndef __GOOFInt3_H__
#define __GOOFInt3_H__

#include <OgreVector3.h>

namespace GOOF
{
	class Int3
	{
	public:
		int x, y, z;

	public:

    public:
        /** Default constructor.
         @note
         It does NOT initialize the vector for efficiency.
        */
        inline Int3()
        {
        }

        inline Int3( const int fX, const int fY, const int fZ )
            : x( fX ), y( fY ), z( fZ )
        {
        }

        inline explicit Int3( const Ogre::Real afCoordinate[3] )
        {           
			x = afCoordinate[0]>0? int(afCoordinate[0]+0.5) : int(afCoordinate[0]-0.5);
			y = afCoordinate[1]>0? int(afCoordinate[1]+0.5) : int(afCoordinate[1]-0.5);
			z = afCoordinate[2]>0? int(afCoordinate[2]+0.5) : int(afCoordinate[2]-0.5);
        }
				
        inline explicit Int3( const int afCoordinate[3] )
        {
            x = (int)afCoordinate[0];
            y = (int)afCoordinate[1];
            z = (int)afCoordinate[2];
        }

        inline explicit Int3( int* const r )
            : x( r[0] ), y( r[1] ), z( r[2] )
        {
        }

        inline explicit Int3( const int interger )
            : x( interger )
            , y( interger )
            , z( interger )
        {
        }

        inline Int3( const Int3& rkVector )
            : x( rkVector.x ), y( rkVector.y ), z( rkVector.z )
        {
        }

		inline int operator [] ( const size_t i ) const
        {
            assert( i < 3 );

            return *(&x+i);
        }

		inline int& operator [] ( const size_t i )
        {
            assert( i < 3 );

            return *(&x+i);
        }
		// Pointer accessor for direct copying
		inline int* ptr()
		{
			return &x;
		}
		// Pointer accessor for direct copying
		inline const int* ptr() const
		{
			return &x;
		}

        /** Assigns the value of the other vector.
            @param
                rkVector The other vector
        */
        inline Int3& operator = ( const Int3& rkVector )
        {
            x = rkVector.x;
            y = rkVector.y;
            z = rkVector.z;

            return *this;
        }

        inline Int3& operator = ( const int finterger )
        {
            x = finterger;
            y = finterger;
            z = finterger;

            return *this;
        }

        inline bool operator == ( const Int3& rkVector ) const
        {
            return ( x == rkVector.x && y == rkVector.y && z == rkVector.z );
        }

        inline bool operator != ( const Int3& rkVector ) const
        {
            return ( x != rkVector.x || y != rkVector.y || z != rkVector.z );
        }

        // arithmetic operations
        inline Int3 operator + ( const Int3& rkVector ) const
        {
            return Int3(
                x + rkVector.x,
                y + rkVector.y,
                z + rkVector.z);
        }

        inline Int3 operator - ( const Int3& rkVector ) const
        {
            return Int3(
                x - rkVector.x,
                y - rkVector.y,
                z - rkVector.z);
        }

        inline Int3 operator * ( const int fInterger ) const
        {
            return Int3(
                x * fInterger,
                y * fInterger,
                z * fInterger);
        }

        inline Int3 operator * ( const Int3& rhs) const
        {
            return Int3(
                x * rhs.x,
                y * rhs.y,
                z * rhs.z);
        }

        inline Int3 operator / ( const int fInterger ) const
        {
            assert( fInterger != 0);

            return Int3(
                x / fInterger,
                y / fInterger,
                z / fInterger);
        }

        inline Int3 operator / ( const Int3& rhs) const
        {
            return Int3(
                x / rhs.x,
                y / rhs.y,
                z / rhs.z);
        }

        inline const Int3& operator + () const
        {
            return *this;
        }

        inline Int3 operator - () const
        {
            return Int3(-x, -y, -z);
        }

        // overloaded operators to help Int3
        inline friend Int3 operator * ( const int fInterger, const Int3& rkVector )
        {
            return Int3(
                fInterger * rkVector.x,
                fInterger * rkVector.y,
                fInterger * rkVector.z);
        }

        inline friend Int3 operator / ( const int fInterger, const Int3& rkVector )
        {
            return Int3(
                fInterger / rkVector.x,
                fInterger / rkVector.y,
                fInterger / rkVector.z);
        }

        inline friend Int3 operator + (const Int3& lhs, const int rhs)
        {
            return Int3(
                lhs.x + rhs,
                lhs.y + rhs,
                lhs.z + rhs);
        }

        inline friend Int3 operator + (const int lhs, const Int3& rhs)
        {
            return Int3(
                lhs + rhs.x,
                lhs + rhs.y,
                lhs + rhs.z);
        }

        inline friend Int3 operator - (const Int3& lhs, const int rhs)
        {
            return Int3(
                lhs.x - rhs,
                lhs.y - rhs,
                lhs.z - rhs);
        }

        inline friend Int3 operator - (const int lhs, const Int3& rhs)
        {
            return Int3(
                lhs - rhs.x,
                lhs - rhs.y,
                lhs - rhs.z);
        }

        // arithmetic updates
        inline Int3& operator += ( const Int3& rkVector )
        {
            x += rkVector.x;
            y += rkVector.y;
            z += rkVector.z;

            return *this;
        }

        inline Int3& operator += ( const int fInterger )
        {
            x += fInterger;
            y += fInterger;
            z += fInterger;
            return *this;
        }

        inline Int3& operator -= ( const Int3& rkVector )
        {
            x -= rkVector.x;
            y -= rkVector.y;
            z -= rkVector.z;

            return *this;
        }

        inline Int3& operator -= ( const int fInterger )
        {
            x -= fInterger;
            y -= fInterger;
            z -= fInterger;
            return *this;
        }

        inline Int3& operator *= ( const int fInterger )
        {
            x *= fInterger;
            y *= fInterger;
            z *= fInterger;
            return *this;
        }

        inline Int3& operator *= ( const Int3& rkVector )
        {
            x *= rkVector.x;
            y *= rkVector.y;
            z *= rkVector.z;

            return *this;
        }

        inline Int3& operator /= ( const int fInterger )
        {
            assert( fInterger != 0 );

            x /= fInterger;
            y /= fInterger;
            z /= fInterger;

            return *this;
        }

        inline Int3& operator /= ( const Int3& rkVector )
        {
            x /= rkVector.x;
            y /= rkVector.y;
            z /= rkVector.z;

            return *this;
        }

		inline bool operator < ( const Int3& rhs ) const
        {
            if( x < rhs.x && y < rhs.y && z < rhs.z )
                return true;
            return false;
        }


        inline bool operator > ( const Int3& rhs ) const
        {
            if( x > rhs.x && y > rhs.y && z > rhs.z )
                return true;
            return false;
        }

		inline bool operator < ( const int& rhs ) const
        {
            if( x < rhs && y < rhs && z < rhs )
                return true;
            return false;
        }


        inline bool operator > ( const int& rhs ) const
        {
            if( x > rhs && y > rhs && z > rhs)
                return true;
            return false;
        }

		inline bool operator <= ( const Int3& rhs ) const
        {
            if( x <= rhs.x && y <= rhs.y && z <= rhs.z )
                return true;
            return false;
        }


        inline bool operator >= ( const Int3& rhs ) const
        {
            if( x >= rhs.x && y >= rhs.y && z >= rhs.z )
                return true;
            return false;
        }

		inline bool operator <= ( const int& rhs ) const
        {
            if( x <= rhs && y <= rhs && z <= rhs )
                return true;
            return false;
        }


        inline bool operator >= ( const int& rhs ) const
        {
            if( x >= rhs && y >= rhs && z >= rhs)
                return true;
            return false;
        }



		inline int squaredLength () const
        {
            return x * x + y * y + z * z;
        }

		/** Returns true if this vector is zero length. */
        inline bool isZeroLength (void) const
        {
			return (x == 0 && y == 0 && z == 0);
        }

		/** Rounds to intergers and assigns to xyz */
		inline void fromVector3(const Ogre::Vector3& value)
		{
			x = value.x>0? int(value.x+0.5) : int(value.x-0.5);
			y = value.y>0? int(value.y+0.5) : int(value.y-0.5);
			z = value.z>0? int(value.z+0.5) : int(value.z-0.5);
		}

		inline Ogre::Vector3 toVector3() const
		{
		    return Ogre::Vector3(x, y, z);
		}
		
		inline explicit Int3( const Ogre::Vector3& rkVector )
        {           
			fromVector3(rkVector);
        }

        inline Ogre::String getAsString() const
        {
            return Ogre::StringConverter::toString( x ) + ", " +
            Ogre::StringConverter::toString( y ) + ", " +
            Ogre::StringConverter::toString( z );
        }
        
		// special points
        static const Int3 ZERO;
        static const Int3 UNIT_X;
        static const Int3 UNIT_Y;
        static const Int3 UNIT_Z;
        static const Int3 NEGATIVE_UNIT_X;
        static const Int3 NEGATIVE_UNIT_Y;
        static const Int3 NEGATIVE_UNIT_Z;
        static const Int3 UNIT_SCALE;
	};
	
	/** for use with maps sets and other containers
	*/
	struct Int3Less
	{
		bool operator()(const Int3& a, const Int3& b) const
		{
			if (a.x < b.x) return true;
			if (a.x > b.x) return false;
			if (a.y < b.y) return true;
			if (a.y > b.y) return false;
			return a.z < b.z;
		}
	};
	
		
}

#endif


