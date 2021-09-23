import java.util.Scanner;
import java.util.Iterator;
import java.util.ListIterator;
import java.util.LinkedList;
import java.util.concurrent.locks.ReentrantLock;

import java.io.*;

class Graph {

	int	s;
	int	t;
	int	n;
	int	m;
	Node	excess;		// list of nodes with excess preflow
	Node	node[];
	Edge	edge[];
	ReentrantLock mutex;

	Graph(Node node[], Edge edge[])
	{
		this.node	= node;
		this.n		= node.length;
		this.edge	= edge;
		this.m		= edge.length;
		mutex = new ReentrantLock();
	}

	synchronized void enter_excess(Node u)
	{
		//mutex.lock();
		if (u != node[s] && u != node[t]) {
			u.next = excess;
			excess = u;
		}
		//mutex.unlock();
	}

	synchronized Node leave_excess()
	{
		Node t = excess;
		if (t != null) excess = t.next;
		return t;
	}

	Node other(Edge a, Node u)
	{
		if (a.u == u)
			return a.v;
		else
			return a.u;
	}

	void relabel(Node u)
	{
		//System.out.println("reladle");
		//u.mutex.lock();
		u.h++;
		//u.mutex.unlock();
		enter_excess(u);
	}

	void push(Node u, Node v, Edge a)
	{
		//System.out.println(" " + u.i + " pushing to " + v.i);
		int d;
		if (u == a.u) {
			d = Math.min(u.e, a.c - a.f);
			a.f += d;
		} else {
			d = Math.min(u.e, a.c + a.f);
			a.f -= d;
		}
		u.e -= d;
		v.e += d;
		if (u.e > 0){
			enter_excess(u);
		}
		if (v.e == d){
			enter_excess(v);
		}
	}


	int preflow(int s, int t) throws Exception
	{
		ListIterator<Edge>	iter;
		int			b;
		Edge			a;
		Node			u;
		Node			v;
		int number_of_threads = 2;

		this.s = s;
		this.t = t;
		node[s].h = n;

		iter = node[s].adj.listIterator();
		while (iter.hasNext()) {
			a = iter.next();

			node[s].e += a.c;
			//System.out.println("gibididid");
			push(node[s], other(a, node[s]), a);
		}

		//tråda här
		Task[] threads = new Task[number_of_threads];

		for (int i = 0; i < number_of_threads; i++){
			threads[i] = new Task(this);
		}

		for (int i = 0; i < number_of_threads; i++){
			threads[i].start();
		}

		for (int i = 0; i < number_of_threads; i++) {
			threads[i].join();
		}



		return node[t].e;
	}
}

class Task extends Thread{
	Graph g;

	Task(Graph g){
		this.g = g;
	}

	public void run()
	{
		ListIterator<Edge>	iter;
		int			b;
		Edge			a;
		Node			u;
		Node			v;

		Boolean ya = true;
		//System.out.println(g.excess);

		while ((u = g.leave_excess()) != null) {
			// g.mutex.lock();
			// u = excess;
			if(ya) System.out.println(u);
			ya = false;
			v = null;
			a = null;
			// excess = u.next;
			// g.mutex.unlock();

			iter = u.adj.listIterator();
			while (iter.hasNext()) {
				a = iter.next();

				if (u == a.u){
					v = a.v;
					b = 1;
				}else {
					v = a.u;
					b = -1;
				}

				if(u.i < v.i){
					u.mutex.lock();
					v.mutex.lock();
				} else {
					v.mutex.lock();
					u.mutex.lock();
				}

				if (u.h > v.h && b*a.f < a.c){
					break;
				}
				else {
					u.mutex.unlock();
					v.mutex.unlock();
					v = null;
				}
			}

			if (v != null){
				g.push(u, v, a);
				u.mutex.unlock();
				v.mutex.unlock();
			}
			else {
				g.relabel(u);
			}
		}
		if(ya) System.out.println("ya");
		System.out.println("job done ");
	}

}

class Node {
	int	h;
	int	e;
	int	i;
	Node	next;
	LinkedList<Edge>	adj;
	ReentrantLock mutex;

	Node(int i)
	{
		this.i = i;
		mutex = new ReentrantLock();
		adj = new LinkedList<Edge>();
	}
}

class Edge {
	Node	u;
	Node	v;
	int	f;
	int	c;

	Edge(Node u, Node v, int c)
	{
		this.u = u;
		this.v = v;
		this.c = c;

	}
}

class Preflow {
	public static void main(String args[]) throws Exception
	{
		System.out.println("oogabooga");
		double	begin = System.currentTimeMillis();
		Scanner s = new Scanner(System.in);
		int	n;
		int	m;
		int	i;
		int	u;
		int	v;
		int	c;
		int	f;
		Graph	g;

		n = s.nextInt();
		m = s.nextInt();
		s.nextInt();
		s.nextInt();
		Node[] node = new Node[n];
		Edge[] edge = new Edge[m];

		for (i = 0; i < n; i += 1)
			node[i] = new Node(i);

		for (i = 0; i < m; i += 1) {
			u = s.nextInt();
			v = s.nextInt();
			c = s.nextInt();
			edge[i] = new Edge(node[u], node[v], c);
			node[u].adj.addLast(edge[i]);
			node[v].adj.addLast(edge[i]);
		}

		g = new Graph(node, edge);
		f = g.preflow(0, n-1);
		double	end = System.currentTimeMillis();
		System.out.println("t = " + (end - begin) / 1000.0 + " s");
		System.out.println("f = " + f);
	}
}
