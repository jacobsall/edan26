#[macro_use] extern crate text_io;

use std::sync::{Mutex,Arc,RwLock};
use std::collections::LinkedList;
use std::cmp;
use std::thread;
use std::collections::VecDeque;
const NBR_THREADS: i32 = 8;

struct Node {
	i:	usize,			/* index of itself for debugging.	*/
	e:	i32,			/* excess preflow.			*/
	h:	i32,			/* height.				*/
}

struct Edge {
        u:      usize,
        v:      usize,
        f:      i32,
        c:      i32,
}

impl Node {
	fn new(ii:usize) -> Node {
		Node { i: ii, e: 0, h: 0 }
	}

}

impl Edge {
        fn new(uu:usize, vv:usize,cc:i32) -> Edge {
                Edge { u: uu, v: vv, f: 0, c: cc }
        }
}

fn enter_excess(excess: &mut VecDeque<usize>, node: &usize, t: &usize){
  if (*node != 0) && (*node != *t) {
    excess.push_back(*node);
  }
}

fn leave_excess(excess: &mut VecDeque<usize>) -> usize{
  return excess.pop_front().unwrap();
}

fn other(u: &usize, edge: &Edge) -> usize{
  if edge.u == *u { return edge.v }
  else { return edge.u }
}

fn relabel(excess: &mut VecDeque<usize>, u: &mut Node, t: &usize){
    println!("Relabling {}", u.i);
    u.h += 1;
    enter_excess(excess, &u.i, &t);
}

fn push(excess: &mut VecDeque<usize>, u: &mut Node, v: &mut Node, e: &mut Edge, t: &usize){
  let d: i32;

  if u.i == e.u{
    d = cmp::min(u.e, e.c - e.f);
    e.f += d;
  } else {
    d = cmp::min(u.e, e.c + e.f);
    e.f -= d;
  }
	println!("pushing {} from {} to {}", d, u.i, v.i);

  u.e -= d;
  v.e += d;

  if u.e > 0 {
    enter_excess(excess, &u.i, &t);
  }

  if v.e == d {
    enter_excess(excess, &v.i, &t);
  }
}


fn main() {
	let n: usize = read!();		/* n nodes.						*/
	let m: usize = read!();		/* m edges.						*/
	let _c: usize = read!();	/* underscore avoids warning about an unused variable.	*/
	let _p: usize = read!();	/* c and p are in the input from 6railwayplanning.	*/
	let mut node = vec![];
	let mut edge = vec![];
	let mut adj: Vec<LinkedList<usize>> =Vec::with_capacity(n);
	let mut excess: VecDeque<usize> = VecDeque::new();
	let debug = false;

	let s = 0;
	let t = n-1;

	println!("n = {}", n);
	println!("m = {}", m);

	for i in 0..n {
		let u:Node = Node::new(i);
		node.push(Arc::new(Mutex::new(u)));
		adj.push(LinkedList::new());
	}

	for i in 0..m {
		let u: usize = read!();
		let v: usize = read!();
		let c: i32 = read!();
		let e:Edge = Edge::new(u,v,c);
		adj[u].push_back(i);
		adj[v].push_back(i);
		edge.push(Arc::new(Mutex::new(e)));
	}

	if debug {
		for i in 0..n {
			print!("adj[{}] = ", i);
			let iter = adj[i].iter();

			for e in iter {
				print!("e = {}, ", e);
			}
			println!("");
		}
	}

	println!("initial pushes");
	let iter = adj[s].iter();
	node[s].lock().unwrap().h = n as i32;
	for e in iter {
		let v = other(&s, &edge[*e].lock().unwrap());
		node[s].lock().unwrap().e += edge[*e].lock().unwrap().c;
		println!("haj du {}", node[s].lock().unwrap().e);

		push(&mut excess, &mut node[s].lock().unwrap(), &mut node[v].lock().unwrap(), &mut edge[*e].lock().unwrap(), &t);
	}
	// but nothing is done here yet...

	let excess_arc = Arc::new(Mutex::new(excess));
	let node_arc = Arc::new(RwLock::new(node));
	let edge_arc = Arc::new(RwLock::new(edge));
	let adj_arc = Arc::new(RwLock::new(adj));
	let mut threads = vec![];

	for _ in 0..NBR_THREADS{
		let excess_main = excess_arc.clone();
		let node_main = Arc::clone(&node_arc);
		let edge_main = Arc::clone(&edge_arc);
		let adj_main = Arc::clone(&adj_arc);

		let h = thread::spawn(move || {
			println!("starting thread");
			let mut b: i32;
			let mut u: usize;
		    let mut v: usize;
			let mut _e_index = 0;
			let mut iter;

			let node_thread = node_main.read().unwrap();
			let edge_thread = edge_main.read().unwrap();
			let adj_thread = adj_main.read().unwrap();

			loop {
				println!("start of loop: 1");
				{
					let mut current_excess = excess_main.lock().unwrap();

					if current_excess.is_empty() {
						break;
					}
					u = leave_excess(&mut current_excess);
				}

				v = n;
				iter = adj_thread[u].iter();

				//let mut u_node;
				//let mut v_node;
				let mut edge;
				let mut e_f;
				let mut e_c;

				for e in iter {

				println!("2");
				edge = edge_thread[*e].lock().unwrap();
				v = other(&u, &edge);

				println!("3");

				if u != edge.u {
					//v = edge.u;
					b = -1;
				} else {
					b = 1;
					//v = edge.v;
				}

				println!("4");

				if u < v {
					println!("u less than v: 5");
				  let mut u_node = node_thread[u].lock().unwrap();
				  let mut v_node = node_thread[v].lock().unwrap();
				  println!("6");
				  e_f = edge.f;
				  e_c = edge.c;

				  if (u_node.h > v_node.h)  && (b * e_f < e_c) {
					  println!("pushing: 7");
					  push(&mut excess_main.lock().unwrap(), &mut u_node, &mut v_node, &mut edge, &t);
				    break;
				  }
				  else  {
					  println!("NOT pushing: 7");
				    v = n;
				  }
				} else {
					println!("u IS NOT less than v: 5");
				  let mut v_node = node_thread[v].lock().unwrap();
				  let mut u_node = node_thread[u].lock().unwrap();
				  println!("6");
				  e_f = edge.f;
				  e_c = edge.c;

				  if (u_node.h > v_node.h)  && (b * e_f < e_c) {
					  println!("pushing: 7");
					  push(&mut excess_main.lock().unwrap(), &mut u_node, &mut v_node, &mut edge, &t);
					  break;
				  }
				  else  {
					  println!("NOT pushing: 7");
				    v = n;
				  }
				}

				}
				if v == n {
					println!("relabel: 8");
					let mut u_node = node_thread[u].lock().unwrap();
				  relabel(&mut excess_main.lock().unwrap(), &mut u_node, &t);
				}
			}
		});
		threads.push(h);
	}

	for h in threads {
		h.join().unwrap();
	}

	println!("f = {}", node_arc.read().unwrap()[t].lock().unwrap().e);

}
