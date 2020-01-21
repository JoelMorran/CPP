#pragma once

#include <smmintrin.h>

class Vector3
{
public:
    Vector3(const Vector3& other) = default;

    Vector3(Vector3&& other) noexcept = default;

    Vector3& operator=(const Vector3& other) = default;

    Vector3& operator=(Vector3&& other) noexcept = default;

	// *** Insert implementation here!
	// *** ACCESSORS ***

	// *** TASK 1. CONSTRUCTORS. ***
	
	// Constructors
	Vector3()
	{
		_vector = _mm_setzero_ps();
	}

	explicit Vector3(const __m128& value)
	{
		_vector = value;
	}

	Vector3(const float x, const float y, const float z)
	{
		_vector = _mm_setr_ps(x, y, z, 0);

	}

	Vector3(const float x, const float y, const float z, const float r)
	{
		_vector = _mm_setr_ps(x, y, z, r);

	}

	explicit Vector3(const float value)
	{
		_vector = _mm_set1_ps(value);
	}

	Vector3 getR()
	{
		return Vector3(_mm_permute_ps(_vector, _MM_SHUFFLE(3,3,3,3)));
	}

	void setR(const Vector3 r)
	{
		_vector = _mm_insert_ps(_vector, r._vector, 0x30);//0011 0000
	}


	

	// *** TASK 2. VECTOR ADDITION. ***
	
	Vector3 operator+ (const Vector3& other) const
	{
		return Vector3 (_mm_add_ps(this->_vector, other._vector ));
	}

	Vector3 operator+ (const float value) const
	{
		return Vector3(_mm_add_ps(this->_vector, _mm_set1_ps(value)));
	}


	

	Vector3& operator+= (const Vector3& other)
	{
		this->_vector = _mm_add_ps(this->_vector, other._vector);
		return *this;
	}
	

	// *** TASK 3. VECTOR SUBTRACTION. ***
	
	Vector3 operator- (const Vector3& other) const
	{
		return Vector3(_mm_sub_ps(this->_vector, other._vector));
	}

	Vector3 operator- (const float value) const
	{
		return Vector3(_mm_sub_ps(this->_vector, _mm_set1_ps(value)));
	}



	Vector3& Vector3::operator-= (const Vector3& other)
	{
		this->_vector = _mm_sub_ps(this->_vector, other._vector);
		return *this;
	}


	//&
	Vector3 operator& (const Vector3& other) const
	{
		return Vector3(_mm_and_ps(this->_vector, other._vector));
	}

	//lessthan
	Vector3 lessThan (const Vector3& other) const
	{
		return Vector3(_mm_cmplt_ps(this->_vector, other._vector));
	}
	

	// *** TASK 4. MULTIPLYING A VECTOR BY A SCALAR ***
	
	Vector3 operator* (const float value) const
	{
		return Vector3(_mm_mul_ps(this->_vector, _mm_set1_ps(value)));
	}

	Vector3 operator* (const Vector3& other) const
	{
		return Vector3(_mm_mul_ps(this->_vector, other._vector));
	}

	Vector3& operator*= (const float value)
	{
		this->_vector = _mm_mul_ps(this->_vector, _mm_set1_ps(value));
		return *this;
	}
	

	// *** TASK 5. DIVIDING A VECTOR BY A SCALAR ***
	
	Vector3 operator/ (const float value) const
	{
		return Vector3(_mm_div_ps(this->_vector, _mm_set1_ps(value)));
	}

	Vector3 operator/ (const Vector3& other) const
	{
		return Vector3(_mm_div_ps(this->_vector, other._vector));
	}

	Vector3& operator/= (const float value)
	{
		this->_vector = _mm_div_ps(this->_vector, _mm_set1_ps(value));
		return *this;
	}

	bool operator< (const Vector3& other) const
	{
		return _mm_comilt_ss(this->_vector, other._vector) == 1;
	}
	

	// *** TASK 6. DOT PRODUCT. ***
	
	Vector3 dot3(const Vector3& other) const
	{
		return Vector3(_mm_dp_ps(this->_vector, other._vector, 0x7F ));
	}
	

	// *** TASK 7. VECTOR LENGTH. ***
	
	Vector3 length() const
	{
		return Vector3(_mm_sqrt_ps(_mm_dp_ps(this->_vector, this->_vector, 0x7F)));
	}
	

	// *** TASK 8. VECTOR NORMALISATION. ***
	
	Vector3 normalise() const
	{
		__m128 temp = _mm_sqrt_ps(_mm_dp_ps(this->_vector, this->_vector, 0x77));
		return Vector3(_mm_div_ps(this->_vector, temp));
	}
	

	// *** TASK 9. CROSS PRODUCT. ***
	
	Vector3 cross3(const Vector3& other) const
	{
		//x=0, y=1, z=2
		/*return Vector3(
			(_vector.y * other._vector.z) - (_vector.z * other._vector.y),
			(_vector.z * other._vector.x) - (_vector.x * other._vector.z),
			(_vector.x * other._vector.y) - (_vector.y * other._vector.x));*/
		__m128 temp1 = _mm_shuffle_ps(this->_vector, this->_vector, _MM_SHUFFLE(3, 0, 2, 1));
		__m128 temp2 = _mm_shuffle_ps(other._vector, other._vector, _MM_SHUFFLE(3, 1, 0, 2));
		__m128 temp3 = _mm_shuffle_ps(this->_vector, this->_vector, _MM_SHUFFLE(3, 1, 0, 2));
		__m128 temp4 = _mm_shuffle_ps(other._vector, other._vector, _MM_SHUFFLE(3, 0, 2, 1));
		return Vector3(_mm_sub_ps(_mm_mul_ps(temp1, temp2), _mm_mul_ps(temp3, temp4)));
	}
	
	inline void* operator new[](size_t p)
	{
		return _aligned_malloc(p, 16);
	}

	inline void operator delete[](void* p)
	{
		if (p != NULL)
		{
			_aligned_free(p);
		}
	}

	// *** End of implementation

	// *** End of implementation
private:
	__m128 _vector;
};
