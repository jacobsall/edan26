import scala.util._
import java.util.Scanner
import java.io._
import akka.actor._
import akka.pattern.ask
import akka.util.Timeout
import scala.concurrent.{Await,ExecutionContext,Future,Promise}
import scala.concurrent.duration._
import scala.language.postfixOps
import scala.io._

case class Flow(f: Int)
case class Debug(debug: Boolean)
case class Control(control:ActorRef)
case class Source(n: Int)
case class Push(a: Edge, h: Int, pf: Int)
case class Ack(r: Int)
case class Bye(e: Int)

case object Print
case object Stop
case object Start
case object Excess
case object Maxflow
case object Sink
case object Hello
case object Nack

class Edge(var u: ActorRef, var v: ActorRef, var c: Int, var idU: Int, var idV: Int) {
	var	f = 0
}

class Node(val index: Int) extends Actor {
	var	e = 0;				/* excess preflow. 						*/
	var	h = 0;				/* height. 							*/
	var	control:ActorRef = null		/* controller to report to when e is zero. 			*/
	var	source:Boolean	= false		/* true if we are the source.					*/
	var	sink:Boolean	= false		/* true if we are the sink.					*/
	var	edge: List[Edge] = Nil		/* adjacency list with edge objects shared with other nodes.	*/
	var	debug = false			/* to enable printing.						*/
	var edgesLeft:List[Edge] = Nil
	var isPushing = false

	def min(a:Int, b:Int) : Int = { if (a < b) a else b }

	def id: String = "@" + index;

	def other(a:Edge, u:ActorRef) : ActorRef = { if (u == a.u) a.v else a.u }

	def status: Unit = { if (true) println(id + " e = " + e + ", h = " + h) }

	def enter(func: String): Unit = { if (debug) { println(id + " enters " + func); status } }
	def exit(func: String): Unit = { if (debug) { println(id + " exits " + func); status } }

	def dbPrint(func: String): Unit = {if (true) { println(id + func); } }


	def relabel : Unit = {
		enter("relabel")
		h += 1
		//dbPrint(" increasing height to " + h);
		exit("relabel")
	}

	def discharge: Unit = {
		//enter("disharge")
		if(e > 0 && !sink && !source && !isPushing){
			if (edgesLeft == Nil){
				relabel
				edgesLeft = edge
			}
			var a = edgesLeft.head
			edgesLeft = edgesLeft.tail

			var pf = if (a.u == self) e else -e
			var otherId = if(a.u == self) a.idV else a.idU
			other(a,self) ! Push(a, h, pf)
			isPushing = true
			//dbPrint(" trying to push " + pf + " to @" + otherId)
		}
		//exit("disharge")
	}

	def receive = {

	case Push(a: Edge, h: Int, pf: Int) => {
		if(h > this.h){
			if(source ) println("hfbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb")
			var otherId = if(a.u == self) a.idV else a.idU
			if (pf > 0){
				var pushable = min(pf, a.c - a.f);
				a.f += pushable
				e += pushable

				//dbPrint(" has accepted " + pushable + " from @" + otherId)
				sender ! Ack(pushable)
				if ((sink || source )&& pushable > 0) {
					//dbPrint(" has " + e + " and byebyes " + pushable)
					control ! Bye(pushable)
				}
			}
			else if (pf < 0){
				var subtractable = min(Math.abs(pf), a.f)
				a.f -= subtractable
				e += subtractable
				//dbPrint(" has accepted " + subtractable + " from @" + otherId)
				sender ! Ack(subtractable)
				if ((sink || source) && subtractable > 0) {
					//dbPrint(" has " + e + " and byebyes " + subtractable)
					control ! Bye(subtractable)
				}
			}

			//if(sink) dbPrint(" its me")
			if(!source) assert(e >= 0)

			discharge
		}
		else {
			sender ! Nack
		}
	}

	case Nack => {
		isPushing = false
		if(!source) assert(e > 0)
		discharge
	}

	case Ack(r: Int) => {
		isPushing = false
		e -= r
		//dbPrint(" and now " + e + " left")
		if(!source) assert(e >= 0)
		if(source && r>0) control ! Bye(-r)
		discharge
	}

	case Hello => {
		//control ! Bye(10932)
		for(a <- edge){
			var pf = if (a.u == self) a.c else -a.c
			other(a,self) ! Push(a,h,pf)
			//control ! Bye(-a.c)
		}
	}

	case Debug(debug: Boolean)	=> this.debug = debug

	case Print => status

	case Excess => { sender ! Flow(e) /* send our current excess preflow to actor that asked for it. */ }

	case edge:Edge => {
		this.edge = edge :: this.edge /* put this edge first in the adjacency-list. */
		edgesLeft = edge :: edgesLeft
	}

	case Control(control:ActorRef)	=> this.control = control

	case Sink	=> { sink = true }

	case Source(n:Int)	=> { h = n; source = true }

	case _		=> {
		println("" + index + " received an unknown message" + _) }

		assert(false)
	}

}


class Preflow extends Actor
{
	var	s	= 0;			/* index of source node.					*/
	var	t	= 0;			/* index of sink node.					*/
	var	n	= 0;			/* number of vertices in the graph.				*/
	var	edge:Array[Edge]	= null	/* edges in the graph.						*/
	var	node:Array[ActorRef]	= null	/* vertices in the graph.					*/
	var	ret:ActorRef 		= null	/* Actor to send result to.					*/
	var total = 0

	def receive = {

	case node:Array[ActorRef]	=> {
		this.node = node
		n = node.size
		s = 0
		t = n-1
		for (u <- node)
			u ! Control(self)
	}

	case edge:Array[Edge] => this.edge = edge

	case Flow(f:Int) => {
		ret ! f			/* somebody (hopefully the sink) told us its current excess preflow. */
	}

	case Maxflow => {
		ret = sender

		node(s) ! Source(n)
		node(t) ! Sink
		node(s) ! Hello

		//node(t) ! Excess	/* ask sink for its excess preflow (which certainly still is zero). */
	}

	case Bye(e: Int) => {
			total += e
			println(total)
			if(total == 0) {
				for (n<-node) n ! Print
				node(t) ! Excess
			}
	}

	}
}

object main extends App {
	implicit val t = Timeout(30 seconds);

	val	begin = System.currentTimeMillis()
	val system = ActorSystem("Main")
	val control = system.actorOf(Props[Preflow], name = "control")

	var	n = 0;
	var	m = 0;
	var	edge: Array[Edge] = null
	var	node: Array[ActorRef] = null

	val	s = new Scanner(System.in);

	n = s.nextInt
	m = s.nextInt

	/* next ignore c and p from 6railwayplanning */
	s.nextInt
	s.nextInt

	node = new Array[ActorRef](n)

	for (i <- 0 to n-1)
		node(i) = system.actorOf(Props(new Node(i)), name = "v" + i)

	edge = new Array[Edge](m)

	for (i <- 0 to m-1) {

		val u = s.nextInt
		val v = s.nextInt
		val c = s.nextInt

		edge(i) = new Edge(node(u), node(v), c, u, v)

		node(u) ! edge(i)
		node(v) ! edge(i)
	}

	control ! node
	control ! edge

	val flow = control ? Maxflow
	val f = Await.result(flow, t.duration)

	println("f = " + f)

	system.stop(control)
	system.terminate()

	val	end = System.currentTimeMillis()

	println("t = " + (end - begin) / 1000.0 + " s")
}
