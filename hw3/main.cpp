#include <iostream>
using namespace std;

void bcast(int dim,int myId, int srcId){
	myId = myId ^ srcId;
	int mask = (1<<dim) - 1;
	for (int i=dim-1;i>=0;i--){
		cout << "i : " << i << endl;
		mask = mask ^ (1<<i);
		if ((myId & mask) == 0){
			if ((myId & (1<<i)) == 0){
				int dest = myId ^ (1<<i);
				dest = dest ^ srcId;
				cout << "sending to destination : " << dest << endl; 
			} else{
				int src = myId ^ (1<<i);
				src = src ^ srcId;
				cout << "recieving from source : " << src << endl; 
			}
		}
		cout << "\n\n";
	}
}


void bcast_n_nodes(int dim,int myId, int n){
	int mask = (1<<dim) - 1;
	for (int i=dim-1;i>=0;i--){
		cout << "i : " << i << endl;
		mask = mask ^ (1<<i);
		if ((myId & mask) == 0){
			if ((myId & (1<<i)) == 0){
				int dest = myId ^ (1<<i);
				if (dest < n)
					cout << "sending to destination : " << dest << endl; 
			} else{
				int src = myId ^ (1<<i);
				cout << "recieving from source : " << src << endl; 
			}
		}
		cout << "\n";
	}
}

void butterfly(int d, int* data){
	int new_data[(1<<d)];
	for (int i=0;i<(1<<d);i++){
		new_data[i] = data[i];
		cout << data[i] << "\t";
	}
	cout << "\n\n";
	for (int i=0;i<d;i++){
		for (int j=0;j<(1<<d);j++){
			int partner = j ^ (1<<i);
			new_data[j] += data[partner];
		}

		for (int k=0;k<(1<<d);k++){
			data[k] = new_data[k];
			cout << new_data[k] << "\t";
		}
		cout << "\n\n";
	}
}

int main(){
	// int dim = 3;
	// for (int i=0;i<5;i++){
	// 	cout << "------Process : "<< i << " -------------\n";
	// 	bcast_n_nodes(dim,i,5);
	// }

	int data[8] = {0,1,2,3,4,5,6,7};
	butterfly(3,data);
}