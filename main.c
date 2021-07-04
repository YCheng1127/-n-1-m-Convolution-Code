#include <stdio.h>
#include <math.h>


/* Viterbi Decoder for (n,1,v) convolution code By Q36094112*/

#define STLEN 14
#define m 7		//memory_order
#define n 3
// k = 1

#define Queue_Length 1700	//>=3

//impulse response
static int g[n][m + 1] = { { 1,0,0,1,0,1,0,1 }, 
					{ 1,1,0,1,1,0,0,1 }, 
					{ 1,1,1,1,0,1,1,1 } };


/*-----Basic Bit Operation-----*/
void Dec2bin(int x, int* bin, int size) {

	int c = 0x00000001;
	for (int i = 0; i < size; i++) {
		bin[size - 1 - i] = (x >> i)&c;
	}
}

int conv(int*a, int*b, int size) {
	int sum = 0;
	for (int i = 0; i < size; i++) {
		sum += a[i] * b[i];
	}
	return sum % 2;
}

int compare_bit(int* a, int* b, int ai, int bi, int size) {

	int sum = 0;

	for (int i = 0; i < size; i++) {
		if (a[ai + i] != b[bi + i])
			sum++;
	}
	return sum;
}


/*-----Bit Array with dynamic length-----*/
struct BitArray {

	int length;
	int index;
	int* data;

};

struct BitArray* New_BitArray() {
	
	struct BitArray* a = malloc(sizeof(struct BitArray));
	a->length = STLEN;
	a->data = malloc(sizeof(int)*a->length);
	a->index = 0;
	return a;
}

void Reverse(struct BitArray* S) {
	int times = S->index / 2;
	for (int i = 0; i < times; i++) {
		S->data[i] = S->data[i] ^ S->data[S->index - 1 - i];
		S->data[S->index - 1 - i] = S->data[i] ^ S->data[S->index - 1 - i];
		S->data[i] = S->data[i] ^ S->data[S->index - 1 - i];;
	}
}

void Addbit(struct BitArray* S, int b) {

	S->data[S->index++] = b;
	if (S->index == S->length) {
		S->length *= 2;
		S->data = realloc(S->data, sizeof(int)*S->length);
	}
}

void Delete_BitArray(struct BitArray* S) {

	free(S->data);
	free(S);
}

void Print_BitArray(struct BitArray* S) {

	for (int i = 0; i < S->index; i++)
		printf("%d", S->data[i]);
}
/*-----State Diagram-----*/
struct statenode {

	int output [2][n];
	int nextstate [2];

};

struct statenode* Build_State_Diagram() {

	int statenum = pow(2, m);
	int one_shift = pow(2, m - 1);
	struct statenode* graph = malloc(sizeof(struct statenode)*statenum);
	for (int i = 0; i < statenum; i++) {

		graph[i].nextstate[1] = i / 2 + one_shift;
		graph[i].nextstate[0] = i / 2;
		int temp[m + 1] = { 0 };
		Dec2bin(i, temp + 1, m);

		for (int j = 0; j < 2; j++) {
			temp[0] = (j) ? 1 : 0;
			for (int r = 0; r < n; r++) {
				graph[i].output[j][r] = conv(temp, g[r], m + 1);
			}
		}
	}

	return graph;
};


/*-----Trellis Diagram-----*/
struct vertex {

	int cost;
	int prev;
	int prev_to_cur;
	int covered;
};


struct vertex_array {

	int minimum_cost;
	int covered_num;
	struct vertex* states;
	int min_cost_state;
};


//Encoder
struct BitArray* Encoder(struct BitArray* input) {

	printf("\n\n\nStart Encoding...");

	int* reverse_input = malloc(sizeof(int)*(input->index + m));
	memset(reverse_input, 0, sizeof(int)*(input->index + m));
	for (int i = 0; i < input->index; i++) {
		reverse_input[input->index - 1 - i] = input->data[i];
	}

	struct BitArray* Output_data = New_BitArray();
	/*
	int output_length = input->index * n;
	int* output = malloc(sizeof(int)*output_length);
	int oi = 0;
	*/
	for (int i = input->index - 1; i >= 0; i--) {
		int sum[n] = { 0 };

		for (int j = 0; j <= m; j++) {
			for (int u = 0; u < n; u++) {
				sum[u] += reverse_input[i + j] * g[u][j];
			}
		}

		for (int u = 0; u < n; u++) {
			Addbit(Output_data, sum[u] % 2);
		}
	}
	printf("\n\n\nThe Output of Encoder is: ");
	Print_BitArray(Output_data);

	return Output_data;
}

struct BitArray* Decoder(struct BitArray* input) {
	//Construct State diagram information
	struct statenode* graph = Build_State_Diagram(g);
	printf("\nFinished Constructing State Diagram\n");



	/*----Decode----*/

	struct BitArray* Output = New_BitArray();


	//Initialize the Queue for Trellis Diagram using a queue of each data is a pow(2,m) array of vertex  
	printf("\nStart Decoding...\n");
	int statenum = pow(2, m);
	int terminate_time = input->index / n;
	int qfront = 0; //first element position
	int qrear = 0;	//new storing position

	struct vertex_array* trellis_D = malloc(sizeof(struct vertex_array)*Queue_Length);
	for (int i = 0; i < Queue_Length; i++) {

		trellis_D[i].covered_num = 0;
		trellis_D[i].minimum_cost = 0x7FFFFFFF;
		trellis_D[i].min_cost_state = -1;
		trellis_D[i].states = malloc(sizeof(struct vertex)*statenum);
		for (int j = 0; j < statenum; j++) {
			trellis_D[i].states[j].cost = 0x7FFFFFFF;
			trellis_D[i].states[j].prev = -1;
			trellis_D[i].states[j].prev_to_cur = -1;
			trellis_D[i].states[j].covered = 0;
		}
	}
	trellis_D[0].states[0].cost = 0; //start point
	trellis_D[0].minimum_cost = 0;
	trellis_D[0].min_cost_state = 0;
	qrear = 1;


	//Walk the Trellis Diagram
	for (int t = 0; t < terminate_time; t++) {


		int cur_state_index = (qrear + Queue_Length - 1) % Queue_Length;

		for (int j = 0; j < statenum; j++) {

			if (trellis_D[cur_state_index].states[j].cost != 0x7FFFFFFF) {	//if the vertex is a starting state

				//Update the possible next state cost(0 or 1)
				for (int r = 0; r < 2; r++) {

					int nextstate = graph[j].nextstate[r];
					int cost = compare_bit(input->data, graph[j].output[r], t*n, 0, n);

					if (cost + trellis_D[cur_state_index].states[j].cost < trellis_D[qrear].states[nextstate].cost) {
						trellis_D[qrear].states[nextstate].cost = cost + trellis_D[cur_state_index].states[j].cost;
						trellis_D[qrear].states[nextstate].prev = j;
						trellis_D[qrear].states[nextstate].prev_to_cur = r;
					}
				}

			}
		}

		//update the information of the current vertex_array(used for finding victim when queue is full)
		for (int j = 0; j < statenum; j++) {

			if (trellis_D[qrear].minimum_cost > trellis_D[qrear].states[j].cost) {
				trellis_D[qrear].minimum_cost = trellis_D[qrear].states[j].cost;
				trellis_D[qrear].min_cost_state = j;
			}
		}

		qrear = (qrear + 1) % Queue_Length;
		//
		if (qrear == qfront) {	//reach the end of queue

			int i = (qrear - 1 + Queue_Length) % Queue_Length;
			int i_prev = (i - 1 + Queue_Length) % Queue_Length;
			int flag = 0;
			int victim_state;
			int reset_index = i_prev;
			//printf("\nThe end index is: %d\n", i);

			trellis_D[i].covered_num = statenum;
			for (int j = 0; j < statenum; j++) {
				trellis_D[i].states[j].covered = 1;
			}

			//find victim and output the sequence then reset
			while (i != qfront) {

				//i_prev = (i - 1 + Queue_Length) % Queue_Length;
				for (int j = 0; j < statenum; j++) {

					if (trellis_D[i].states[j].covered == 1) {
						int cover_pt = trellis_D[i].states[j].prev;
						trellis_D[i_prev].states[cover_pt].covered = 1;
						victim_state = cover_pt;
					}
				}

				for (int j = 0; j < statenum; j++) {
					if (trellis_D[i_prev].states[j].covered) trellis_D[i_prev].covered_num++;
				}

				if (trellis_D[i_prev].covered_num == 1) { flag = 1;  break; }
				else {
					i = i_prev;
					i_prev = (i_prev - 1 + Queue_Length) % Queue_Length;
				}

			}
			reset_index = flag ? i_prev : reset_index;
			victim_state = flag ? victim_state : trellis_D[reset_index].min_cost_state;

			//printf("\nThe reset_index is %d\n", reset_index);

			//Output
			int c = reset_index;
			int cur = victim_state;
			struct BitArray* temp = New_BitArray();
			while (c != (qfront - 1 + Queue_Length) % Queue_Length) {

				int bit = trellis_D[c].states[cur].prev_to_cur;
				if (bit != -1)Addbit(temp, bit);

				cur = trellis_D[c].states[cur].prev;

				trellis_D[c].minimum_cost = 0x7FFFFFFF;
				trellis_D[c].min_cost_state = -1;
				for (int j = 0; j < statenum; j++) {
					trellis_D[c].states[j].cost = 0x7FFFFFFF;
					trellis_D[c].states[j].prev = -1;
					trellis_D[c].states[j].prev_to_cur = -1;
				}

				c = (c - 1 + Queue_Length) % Queue_Length;

			}

			Reverse(temp);
			for (int u = 0; u < temp->index; u++) {
				Addbit(Output, temp->data[u]);
			}
			Delete_BitArray(temp);

			/*
			printf("\n");
			for (int i = 0; i < Output->index; i++) {
				printf("%d", Output->data[i]);
			}
			printf("\n");
			*/

			c = (qrear - 1 + Queue_Length) % Queue_Length;
			while (c != (qfront - 1 + Queue_Length) % Queue_Length) {
				trellis_D[c].covered_num = 0;
				for (int j = 0; j < statenum; j++)
					trellis_D[c].states[j].covered = 0;
			}

			qfront = (reset_index + 1) % Queue_Length;
		}
		//
	}

	int c = (qrear - 1 + Queue_Length) % Queue_Length;
	int prev = trellis_D[c].min_cost_state;

	struct BitArray* temp = New_BitArray();

	while (c != (qfront - 1 + Queue_Length) % Queue_Length) {

		int bit = trellis_D[c].states[prev].prev_to_cur;
		if (bit != -1)Addbit(temp, trellis_D[c].states[prev].prev_to_cur);
		prev = trellis_D[c].states[prev].prev;
		c = (c - 1 + Queue_Length) % Queue_Length;
	}
	Reverse(temp);
	for (int i = 0; i < temp->index; i++) {
		Addbit(Output, temp->data[i]);
	}
	Delete_BitArray(temp);

	for (int i = 0; i < m; i++) {
		Output->data[Output->index - 1 - i] = 0;
	}

	printf("\n\nThe Output of Decoder is: ");
	for (int i = 0; i < Output->index; i++) {
		printf("%d", Output->data[i]);
	}

	return Output;
}

int main(int argc, char* argv) {

	//Get input
	int buffersize = STLEN;
	char* buffer = malloc(sizeof(char) * buffersize);
	int bi = 0;
	printf("Please enter the received sequence: ");
	buffer[bi++] = getchar();
	while (buffer[bi - 1] != '\n') {

		if (bi == buffersize - 1) {
			buffersize *= 2;
			buffer = realloc(buffer, sizeof(char)*buffersize);
		}
		buffer[bi++] = getchar();

	}
	buffer[bi - 1] = '\0';

	int input_length = bi - 1;
	printf("\n\nThe received sequence is %s\n\n\n", buffer);

	//transfer string to int
	struct BitArray* Received_Data = New_BitArray();
	for (int i = 0; i < input_length; i++) {
		Addbit(Received_Data, buffer[i] - 48);
	}
	 
	struct BitArray* Decode_Data = Decoder(Received_Data);
	struct BitArray* Encode_Data = Encoder(Decode_Data);
	

	struct BitArray* Error_pos = New_BitArray();
	int error_sum = 0;
	for (int i = 0; i < Received_Data->index; i++) {
		if (Received_Data->data[i] != Encode_Data->data[i]) {
			error_sum++;
			Addbit(Error_pos, i + 1);
		}
	}

	printf("\n\n\n\n===================================\n\nThe Error num is %d", error_sum);
	printf("\nThe error positions are: ");
	for (int i = 0; i < Error_pos->index; i++) {
		printf("%d ", Error_pos->data[i]);
	}

}

