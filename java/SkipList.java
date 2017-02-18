import java.util.Comparator;
import java.util.Random;

import net.datastructures.*;

public class SkipList<K, V> implements Dictionary<K, V> {
	private Comparator<K> cmp;
	private Position<Entry<K, V>> src;
	private Random rng;
	private K minKey, maxKey;
	private int length, height;

	// the internal Entry class with a key and value
	// (acts like a two-dimensional doubly-linked list)
	protected static class MyEntry<K, V> implements Entry<K, V>, Position<Entry<K, V>> {
		private K k;
		private MyEntry<K, V> prev, next, above, below;
		private V v;

		// constructor
		public MyEntry(K k, V v) {
			this.k = k;
			this.v = v;
		}

		// accessors
		public Entry<K, V> element() { return this; }

		public MyEntry<K, V> getAbove() { return above; }
		public MyEntry<K, V> getBelow() { return below; }
		public MyEntry<K, V> getNext() { return next; }
		public MyEntry<K, V> getPrev() { return prev; }

		public K getKey() { return k; }
		public V getValue() { return v; }

		// updaters
		public void setAbove(MyEntry<K, V> e) { above = e; }
		public void setBelow(MyEntry<K, V> e) { below = e; }
		public void setNext(MyEntry<K, V> e) { next = e; }
		public void setPrev(MyEntry<K, V> e) { prev = e; }

		public String toString() { return ("<" + k  + ", " + v +">"); }
	}

	public SkipList(K minKey, K maxKey) {
		length = 0;
		height = 1;

		cmp = new DefaultComparator<K>();
		rng = new Random();

		this.minKey = minKey;
		this.maxKey = maxKey;

		// instantiate left-most Entry (on bottom level) with -inf
		src = insertAfterAbove(null, null, new MyEntry<K, V>(minKey, null));
		// instantiate +inf key (on bottom level), link -inf to +inf
		insertAfterAbove(src, null, new MyEntry<K, V>(maxKey, null));

		src = insertAfterAbove(null, src, new MyEntry<K, V>(minKey, null));
		insertAfterAbove(src, next(src), new MyEntry<K, V>(maxKey, null));
	}


	public Iterable<Entry<K, V>> entries() {
		PositionList<Entry<K, V>> entries = new NodePositionList<Entry<K, V>>();
		Position<Entry<K, V>> p = src;

		while (below(p) != null)
			p = below(p);

		while (next(p) != null) {
			entries.addLast(p.element());
			p = next(p);
		}

		entries.remove(entries.first());
		return entries;
	}


	public Entry<K, V> find(K k) throws InvalidKeyException {
		Position<Entry<K, V>> p = skipSearch(k);
		return (p.element());
	}

	public Iterable<Entry<K, V>> findAll(K k) throws InvalidKeyException {
		PositionList<Entry<K, V>> entries = new NodePositionList<Entry<K, V>>();
		Position<Entry<K, V>> p = skipSearch(k);

		while( prev(p) != null ) {
			entries.addLast(p.element());
			p = prev(p);

			MyEntry<K, V> e = (MyEntry<K, V>) p.element();

			if (cmp.compare(k, e.getKey()) != 0) {
				break;
			}
		}

		return entries;
	}

	public Entry<K, V> insert(K k, V v) throws InvalidKeyException {
		Position<Entry<K, V>> p = skipSearch(k);
		Position<Entry<K, V>> q = insertAfterAbove(p, null, new MyEntry<K, V>(k, v));

		MyEntry<K, V> e = (MyEntry<K, V>) q.element();

		// perform coin-flips for probabilistic insertion into higher layers
		// initial height is 1; insertion on lowest level must always succeed
		for (int i = 0; i == 0 || rng.nextBoolean(); ++i) {
			if (i >= height) {
				Position<Entry<K, V>> t = next(src);

				src = insertAfterAbove(null, src, new MyEntry<K, V>(this.minKey, null));
				insertAfterAbove(src, t, new MyEntry<K, V>(this.maxKey, null));

				height++;
			}

			while (above(p) == null)
				p = prev(p);

			p = above(p);
			q = insertAfterAbove(p, q, new MyEntry<K, V>(k, v));
		}

		length++;
		return e;
	}

	public Entry<K, V> remove(Entry<K, V> e) throws InvalidEntryException {
		Position<Entry<K, V>> p = skipSearch(e.getKey());
		unset(p);

		while (above(p) != null ) {
			p = above(p);
			unset(p);
		}

		return (p.element());
	}

	public boolean isEmpty() { return (length == 0); }
	public int getLength() { return length; }
	public int getHeight() { return height; }


	private Position<Entry<K, V>> above(Position<Entry<K, V>> p) {
		MyEntry<K, V> e = (MyEntry<K, V>) p.element();
		return (e.getAbove());
	}

	private Position<Entry<K, V>> below(Position<Entry<K, V>> p) {
		MyEntry<K, V> e = (MyEntry<K, V>) p.element();
		return (e.getBelow());
	}



	// inserts an entry e after position p and above q
	//
	// updates the new entry's references as well as
	// the references of all the existing entries to
	// the inserted element e
	//
	private Position<Entry<K, V>> insertAfterAbove(Position<Entry<K, V>> p, Position<Entry<K, V>> q, MyEntry<K, V> e) {
		if (p != null) {
			MyEntry<K, V> eP = (MyEntry<K, V>) p.element();

			if (eP.getNext() != null) {
				MyEntry<K, V> ePnext = eP.getNext();

				e.setNext(ePnext);
				ePnext.setPrev(e);
			}

			e.setPrev(eP);
			eP.setNext(e);
		}

		if (q != null) {
			MyEntry<K, V> eQ = (MyEntry<K, V>) q.element();

			e.setBelow(eQ);
			eQ.setAbove(e);
		}

		return e;
	}

	private K key(Position<Entry<K, V>> p) {
		MyEntry<K, V> e = (MyEntry<K, V>) p.element();
		return (e.getKey());
	}

	private Position<Entry<K, V>> next(Position<Entry<K, V>> p) {
		MyEntry<K, V> e = (MyEntry<K, V>) p.element();
		return (e.getNext());
	}
	
	private Position<Entry<K, V>> prev(Position<Entry<K, V>> p) {
		MyEntry<K, V> e = (MyEntry<K, V>) p.element();
		return (e.getPrev());
	}


	// returns the largest entry's position with the largest key LEQ k
	private Position<Entry<K, V>> skipSearch(K k) {
		Position<Entry<K, V>> p = src;

		while (below(p) != null) {
			p = below(p);

			while (cmp.compare(k, key(next(p))) >= 0) {
				p = next(p);
			}
		}

		return p;
	}

	// unsets an entry given a position p and updates all connected references
	private void unset(Position<Entry<K, V>> p) {
		Position<Entry<K, V>> next = next(p);
		Position<Entry<K, V>> prev = prev(p);

		MyEntry<K, V> eNext = (MyEntry<K, V>) next.element();
		MyEntry<K, V> e     = (MyEntry<K, V>) p.element();
		MyEntry<K, V> ePrev = (MyEntry<K, V>) prev.element();

		e.setBelow(null);
		e.setNext(null);
		e.setPrev(null);

		eNext.setPrev(ePrev);
		ePrev.setNext(eNext);
	}
}

