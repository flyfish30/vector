// The MIT License (MIT)
// Copyright (c) 2016 Peter Goldsborough
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
	
#define __STDC_WANT_LIB_EXT1__ 1

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "vector.h"

int vector_setup(Vector* vector, size_t capacity, size_t element_size) {
	assert(vector != NULL);

	if (vector == NULL) return VECTOR_ERROR;

	vector->size = 0;
	vector->capacity = MAX(VECTOR_MINIMUM_CAPACITY, capacity);
	vector->element_size = element_size;
	vector->data = malloc(vector->capacity * element_size);

	return vector->data == NULL ? VECTOR_ERROR : VECTOR_SUCCESS;
}

int vector_copy(Vector* destination, Vector* source) {
	assert(destination != NULL);
	assert(source != NULL);
	assert(vector_is_initialized(source));
	assert(!vector_is_initialized(destination));

	if (destination == NULL) return VECTOR_ERROR;
	if (source == NULL) return VECTOR_ERROR;
	if (vector_is_initialized(destination)) return VECTOR_ERROR;
	if (!vector_is_initialized(source)) return VECTOR_ERROR;

	/* Copy ALL the data */
	destination->size = source->size;
	if (source->size == 0) {
	    destination->capacity = VECTOR_MINIMUM_CAPACITY;
	} else {
	    destination->capacity = source->size * 2;
	}
	destination->element_size = source->element_size;

	/* Note that we are not necessarily allocating the same capacity */
	destination->data = malloc(destination->capacity * source->element_size);
	if (destination->data == NULL) return VECTOR_ERROR;

	memcpy(destination->data, source->data, vector_byte_size(source));

	return VECTOR_SUCCESS;
}

int vector_copy_assign(Vector* destination, Vector* source) {
	assert(destination != NULL);
	assert(source != NULL);
	assert(vector_is_initialized(source));
	assert(vector_is_initialized(destination));

	if (destination == NULL) return VECTOR_ERROR;
	if (source == NULL) return VECTOR_ERROR;
	if (!vector_is_initialized(destination)) return VECTOR_ERROR;
	if (!vector_is_initialized(source)) return VECTOR_ERROR;

	vector_destroy(destination);

	return vector_copy(destination, source);
}

int vector_move(Vector* destination, Vector* source) {
	assert(destination != NULL);
	assert(source != NULL);

	if (destination == NULL) return VECTOR_ERROR;
	if (source == NULL) return VECTOR_ERROR;

	*destination = *source;
	source->data = NULL;

	return VECTOR_SUCCESS;
}

int vector_move_assign(Vector* destination, Vector* source) {
	vector_swap(destination, source);
	return vector_destroy(source);
}

int vector_swap(Vector* destination, Vector* source) {
	void* temp;

	assert(destination != NULL);
	assert(source != NULL);
	assert(vector_is_initialized(source));
	assert(vector_is_initialized(destination));

	if (destination == NULL) return VECTOR_ERROR;
	if (source == NULL) return VECTOR_ERROR;
	if (!vector_is_initialized(destination)) return VECTOR_ERROR;
	if (!vector_is_initialized(source)) return VECTOR_ERROR;

	_vector_swap(&destination->size, &source->size);
	_vector_swap(&destination->capacity, &source->capacity);
	_vector_swap(&destination->element_size, &source->element_size);

	temp = destination->data;
	destination->data = source->data;
	source->data = temp;

	return VECTOR_SUCCESS;
}

int vector_destroy(Vector* vector) {
	assert(vector != NULL);

	if (vector == NULL) return VECTOR_ERROR;

	free(vector->data);
	vector->data = NULL;

	return VECTOR_SUCCESS;
}

/* Insertion */
int vector_push_back(Vector* vector, void* element) {
	assert(vector != NULL);
	assert(element != NULL);

	if (_vector_should_grow(vector)) {
		if (_vector_adjust_capacity(vector) == VECTOR_ERROR) {
			return VECTOR_ERROR;
		}
	}

	_vector_assign(vector, vector->size, element);

	++vector->size;

	return VECTOR_SUCCESS;
}

int vector_push_front(Vector* vector, void* element) {
	return vector_insert(vector, 0, element);
}

int vector_insert(Vector* vector, size_t index, void* element) {
	void* offset;

	assert(vector != NULL);
	assert(element != NULL);
	assert(index <= vector->size);

	if (vector == NULL) return VECTOR_ERROR;
	if (element == NULL) return VECTOR_ERROR;
	if (vector->element_size == 0) return VECTOR_ERROR;
	if (index > vector->size) return VECTOR_ERROR;

	if (_vector_should_grow(vector)) {
		if (_vector_adjust_capacity(vector) == VECTOR_ERROR) {
			return VECTOR_ERROR;
		}
	}

	/* Move other elements to the right */
	if (_vector_move_right(vector, index) == VECTOR_ERROR) {
		return VECTOR_ERROR;
	}

	/* Insert the element */
	offset = _vector_offset(vector, index);
	memcpy(offset, element, vector->element_size);
	++vector->size;

	return VECTOR_SUCCESS;
}

int vector_assign(Vector* vector, size_t index, void* element) {
	assert(vector != NULL);
	assert(element != NULL);
	assert(index < vector->size);

	if (vector == NULL) return VECTOR_ERROR;
	if (element == NULL) return VECTOR_ERROR;
	if (vector->element_size == 0) return VECTOR_ERROR;
	if (index >= vector->size) return VECTOR_ERROR;

	_vector_assign(vector, index, element);

	return VECTOR_SUCCESS;
}

/* Deletion */
int vector_pop_back(Vector* vector) {
	assert(vector != NULL);
	assert(vector->size > 0);

	if (vector == NULL) return VECTOR_ERROR;
	if (vector->element_size == 0) return VECTOR_ERROR;

	--vector->size;

#ifndef VECTOR_NO_SHRINK
	if (_vector_should_shrink(vector)) {
		_vector_adjust_capacity(vector);
	}
#endif

	return VECTOR_SUCCESS;
}

int vector_pop_front(Vector* vector) {
	return vector_erase(vector, 0);
}

int vector_erase(Vector* vector, size_t index) {
	assert(vector != NULL);
	assert(index < vector->size);

	if (vector == NULL) return VECTOR_ERROR;
	if (vector->element_size == 0) return VECTOR_ERROR;
	if (index >= vector->size) return VECTOR_ERROR;

	/* Just overwrite */
	_vector_move_left(vector, index);

#ifndef VECTOR_NO_SHRINK
	if (--vector->size == vector->capacity / 4) {
		_vector_adjust_capacity(vector);
	}
#endif

	return VECTOR_SUCCESS;
}

int vector_clear(Vector* vector) {
	return vector_resize(vector, 0);
}

/* Lookup */
void* vector_get(Vector* vector, size_t index) {
	assert(vector != NULL);
	assert(index < vector->size);

	if (vector == NULL) return NULL;
	if (vector->element_size == 0) return NULL;
	if (index >= vector->size) return NULL;

	return _vector_offset(vector, index);
}

const void* vector_const_get(const Vector* vector, size_t index) {
	assert(vector != NULL);
	assert(index < vector->size);

	if (vector == NULL) return NULL;
	if (vector->element_size == 0) return NULL;
	if (index >= vector->size) return NULL;

	return _vector_const_offset(vector, index);
}

void* vector_front(Vector* vector) {
	return vector_get(vector, 0);
}

void* vector_back(Vector* vector) {
	return vector_get(vector, vector->size - 1);
}

/* Information */

bool vector_is_initialized(const Vector* vector) {
	return vector->data != NULL;
}

size_t vector_byte_size(const Vector* vector) {
	return vector->size * vector->element_size;
}

size_t vector_free_space(const Vector* vector) {
	return vector->capacity - vector->size;
}

bool vector_is_empty(const Vector* vector) {
	return vector->size == 0;
}

/* Memory management */
int vector_resize(Vector* vector, size_t new_size) {
	if (new_size <= vector->capacity * VECTOR_SHRINK_THRESHOLD) {
		vector->size = new_size;
		if (_vector_reallocate(vector, new_size * VECTOR_GROWTH_FACTOR) == -1) {
			return VECTOR_ERROR;
		}
	} else if (new_size > vector->capacity) {
		if (_vector_reallocate(vector, new_size * VECTOR_GROWTH_FACTOR) == -1) {
			return VECTOR_ERROR;
		}
	}

	vector->size = new_size;

	return VECTOR_SUCCESS;
}

int vector_reserve(Vector* vector, size_t minimum_capacity) {
	if (minimum_capacity > vector->capacity) {
		if (_vector_reallocate(vector, minimum_capacity) == VECTOR_ERROR) {
			return VECTOR_ERROR;
		}
	}

	return VECTOR_SUCCESS;
}

int vector_shrink_to_fit(Vector* vector) {
	return _vector_reallocate(vector, vector->size);
}

/* Iterators */
Iterator vector_begin(Vector* vector) {
	return vector_iterator(vector, 0);
}

Iterator vector_end(Vector* vector) {
	return vector_iterator(vector, vector->size);
}

Iterator vector_iterator(Vector* vector, size_t index) {
	Iterator iterator = {NULL, 0};

	assert(vector != NULL);
	assert(index <= vector->size);

	if (vector == NULL) return iterator;
	if (index > vector->size) return iterator;
	if (vector->element_size == 0) return iterator;

	iterator.pointer = _vector_offset(vector, index);
	iterator.element_size = vector->element_size;

	return iterator;
}

void* iterator_get(Iterator* iterator) {
	return iterator->pointer;
}

int iterator_erase(Vector* vector, Iterator* iterator) {
	size_t index = iterator_index(vector, iterator);

	if (vector_erase(vector, index) == VECTOR_ERROR) {
		return VECTOR_ERROR;
	}

	*iterator = vector_iterator(vector, index);

	return VECTOR_SUCCESS;
}

void iterator_increment(Iterator* iterator) {
	assert(iterator != NULL);
	iterator->pointer += iterator->element_size;
}

void iterator_decrement(Iterator* iterator) {
	assert(iterator != NULL);
	iterator->pointer -= iterator->element_size;
}

void* iterator_next(Iterator* iterator) {
	void* current = iterator->pointer;
	iterator_increment(iterator);

	return current;
}

void* iterator_previous(Iterator* iterator) {
	void* current = iterator->pointer;
	iterator_decrement(iterator);

	return current;
}

bool iterator_equals(Iterator* first, Iterator* second) {
	assert(first->element_size == second->element_size);
	return first->pointer == second->pointer;
}

bool iterator_is_before(Iterator* first, Iterator* second) {
	assert(first->element_size == second->element_size);
	return first->pointer < second->pointer;
}

bool iterator_is_after(Iterator* first, Iterator* second) {
	assert(first->element_size == second->element_size);
	return first->pointer > second->pointer;
}

size_t iterator_index(Vector* vector, Iterator* iterator) {
	assert(vector != NULL);
	assert(iterator != NULL);
	return (iterator->pointer - vector->data) / vector->element_size;
}

/***** PRIVATE *****/

bool _vector_should_grow(Vector* vector) {
	assert(vector->size <= vector->capacity);
	return vector->size == vector->capacity;
}

bool _vector_should_shrink(Vector* vector) {
	assert(vector->size <= vector->capacity);
	return vector->size == vector->capacity * VECTOR_SHRINK_THRESHOLD;
}

size_t _vector_free_bytes(const Vector* vector) {
	return vector_free_space(vector) * vector->element_size;
}

void* _vector_offset(Vector* vector, size_t index) {
	return vector->data + (index * vector->element_size);
}

const void* _vector_const_offset(const Vector* vector, size_t index) {
	return vector->data + (index * vector->element_size);
}

void _vector_assign(Vector* vector, size_t index, void* element) {
	/* Insert the element */
	void* offset = _vector_offset(vector, index);
	memcpy(offset, element, vector->element_size);
}

int _vector_move_right(Vector* vector, size_t index) {
	assert(vector->size < vector->capacity);

	/* The location where to start to move from. */
	void* offset = _vector_offset(vector, index);

	/* How many to move to the right. */
	size_t elements_in_bytes = (vector->size - index) * vector->element_size;

#ifdef __STDC_LIB_EXT1__
	size_t right_capacity_in_bytes =
			(vector->capacity - (index + 1)) * vector->element_size;

	/* clang-format off */
	int return_code =  memmove_s(
		offset + vector->element_size,
		right_capacity_in_bytes,
		offset,
		elements_in_bytes
	);
	/* clang-format on */

	return return_code == 0 ? VECTOR_SUCCESS : VECTOR_ERROR;

#else
	memmove(offset + vector->element_size, offset, elements_in_bytes);
	return VECTOR_SUCCESS;
#endif
}

void _vector_move_left(Vector* vector, size_t index) {
	size_t right_elements_in_bytes;
	void* offset;

	/* The offset into the memory */
	offset = _vector_offset(vector, index);

	/* How many to move to the left */
	right_elements_in_bytes = (vector->size - index - 1) * vector->element_size;

	memmove(offset, offset + vector->element_size, right_elements_in_bytes);
}

int _vector_adjust_capacity(Vector* vector) {
	return _vector_reallocate(vector,
														MAX(1, vector->size * VECTOR_GROWTH_FACTOR));
}

int _vector_reallocate(Vector* vector, size_t new_capacity) {
	size_t new_capacity_in_bytes;
	void* old;
	assert(vector != NULL);

	if (new_capacity < VECTOR_MINIMUM_CAPACITY) {
		if (vector->capacity > VECTOR_MINIMUM_CAPACITY) {
			new_capacity = VECTOR_MINIMUM_CAPACITY;
		} else {
			/* NO-OP */
			return VECTOR_SUCCESS;
		}
	}

	new_capacity_in_bytes = new_capacity * vector->element_size;
	old = vector->data;

	if ((vector->data = malloc(new_capacity_in_bytes)) == NULL) {
		return VECTOR_ERROR;
	}

#ifdef __STDC_LIB_EXT1__
	/* clang-format off */
	if (memcpy_s(vector->data,
							 new_capacity_in_bytes,
							 old,
							 vector_byte_size(vector)) != 0) {
		return VECTOR_ERROR;
	}
/* clang-format on */
#else
	memcpy(vector->data, old, vector_byte_size(vector));
#endif

	vector->capacity = new_capacity;

	free(old);

	return VECTOR_SUCCESS;
}

void _vector_swap(size_t* first, size_t* second) {
	size_t temp = *first;
	*first = *second;
	*second = temp;
}
