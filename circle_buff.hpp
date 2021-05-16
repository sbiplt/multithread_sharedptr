#ifndef CIRCLE_BUFF_H
#define CIRCLE_BUFF_H

template<typename T, int BUFF_SIZE>
class circle_buffer {
private:
	boost::shared_ptr<T> *ptr_buffer;
	int head;
	int rear;
	int size;
public:
	circle_buffer();
	~circle_buffer();
	bool is_empty();

	bool push(boost::shared_ptr<T>&& rptr);

	void pop();

	boost::weak_ptr<T> front();
};

template<typename T, int BUFF_SIZE>
circle_buffer<T, BUFF_SIZE>::circle_buffer() {
	ptr_buffer = new boost::shared_ptr<T>[BUFF_SIZE];
	head = rear = 0;
	size = BUFF_SIZE;
}

template<typename T, int BUFF_SIZE>
circle_buffer<T, BUFF_SIZE>::~circle_buffer()
{
	for (int i = 0; i < BUFF_SIZE; i++)
	{
		ptr_buffer[i] = nullptr;
	}
	delete ptr_buffer;
}


template<typename T, int BUFF_SIZE>
bool circle_buffer<T, BUFF_SIZE>::is_empty()
{
	return rear == head;
}


template<typename T, int BUFF_SIZE>
bool circle_buffer<T, BUFF_SIZE>::push(boost::shared_ptr<T>&& rptr) {
	if ((rear + 1) % size == head) {
		return false;
	}

	ptr_buffer[rear % size] = rptr;
	rear = (rear + 1) % size;

	return true;
}


template<typename T, int BUFF_SIZE>
void circle_buffer<T, BUFF_SIZE>::pop() {
	if (rear == head) {
		return;
	}

	ptr_buffer[head] = nullptr;
	head = (head + 1) % size;
}


template<typename T, int BUFF_SIZE>
boost::weak_ptr<T> circle_buffer<T, BUFF_SIZE>::front() {
	return ptr_buffer[head];
}

#endif 