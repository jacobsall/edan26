#[macro_use] extern crate text_io;

use std::sync::{Mutex,Arc};
use std::collections::LinkedList;
use std::cmp;
use std::thread;
use std::collections::VecDeque;

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

fn enter_excess(excess: &mut VecDeque<usize>, node: &usize, n: &usize){
  //println!("1");
  if (*node != 0) && (*node != n-1) {
  //println!("2");
    excess.push_back(*node);
  }
  //println!("3");
}

fn leave_excess(excess: &mut VecDeque<usize>) -> usize{
  return excess.pop_front().unwrap();
}

fn other(u: &usize, edge: &Edge) -> usize{
  if edge.u == *u { return edge.v }
  else { return *u }
}

fn relabel(excess: &mut VecDeque<usize>, u: &mut Node, n: &usize){
    println!("Relabling {}", u.i);
    u.h += 1;
    enter_excess(excess, &u.i, &n);
}

fn push(excess: &mut VecDeque<usize>, u: &mut Node, v: &mut Node, e: &mut Edge, n: &usize){
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
    enter_excess(excess, &u.i, &n);
  }

  if v.e == d {
    enter_excess(excess, &v.i, &n);
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
  for e in iter {
    let v = other(&s, &edge[*e].lock().unwrap());
    node[s].lock().unwrap().e += edge[*e].lock().unwrap().c;
    println!("haj du {}", node[s].lock().unwrap().e);

    push(&mut excess, &mut node[s].lock().unwrap(), &mut node[v].lock().unwrap(), &mut edge[*e].lock().unwrap(), &n); 
  }
	// but nothing is done here yet...

	while !excess.is_empty() {
		let mut c = 0;
    let mut b = 1;
    let mut v = n;
    let mut e_index = 0; 
		let u = leave_excess(&mut excess);
    //println!("ayy lmao");
	  let iter = adj[u].iter();
    for e in iter {
      e_index = *e;
      v = other(&u, &edge[*e].lock().unwrap());
      if u != *e {
        b = -1;
      }
      
      let u_h = node[u].lock().unwrap().h;    
      let v_h = node[v].lock().unwrap().h;
      let e_f = edge[*e].lock().unwrap().f;
      let e_c = edge[*e].lock().unwrap().c; 
      if ( u_h > v_h)  && (b*e_f<e_c) {
        break;
      }
      else  {
        v = n;
      }
    }
    if v != n {
      push(&mut excess, &mut node[u].lock().unwrap(), &mut node[v].lock().unwrap(), &mut edge[e_index].lock().unwrap(), &n); 
    } else {
      relabel(&mut excess, &mut node[u].lock().unwrap(), &n);
    }
  
	}

	println!("f = {}", node[t].lock().unwrap().e);

}
