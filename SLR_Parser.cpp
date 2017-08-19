#include <bits/stdc++.h>
#include <pthread.h>
#include <mutex>
using namespace std;

#define F first
#define S second

mutex mtx; 

struct Rule {
  char lhs;
  string rhs;
  int id, ptr;
  
  Rule() {
    lhs = '\0';
    rhs = "";
    ptr = 0;
    id = -1;
  }
  
  Rule( char _l, string _r, int _id ): lhs(_l), rhs(_r), id(_id), ptr(0) {}
  
  bool operator<( const Rule &a ) const {
    string p(1,lhs);
    p += rhs;
    p.insert( ptr+1, "." );
    string q(1,a.lhs);
    q += a.rhs;
    q.insert( a.ptr+1, "." );
    return ( p < q );
  }
    
  void print() {
    string q = rhs;
    q.insert( ptr, "." );
    cout << lhs << "->" << q << " :" << ptr << '\n';
  }
  
};

set<char> nonTerm, Term, literals;
set<Rule> RuleSet;
map< char, set<Rule> > LHS;
map< pair<int,char>, pair<char,int> > ACTION;
map<string,int> HASH;
map<int,bool> explored;
map< char, set<char> > _FIRST, _FOLLOW;
map< int, Rule > RuleList;
int statecount = 0, rules = 0;
char start;

int getHash( set<Rule> p ) {
  string str;
  for( Rule r : p ) {
    str.push_back(';');
    string rule;
    rule.push_back(r.lhs);
    rule += "->" + r.rhs;
    rule.insert( r.ptr+3, "." );
    str += rule;
  }
  if( HASH.find(str) == HASH.end() )
    HASH[str] = statecount++;
  return HASH[str];
}

void escape( string &s ) {
  string str;
  for( char c : s )
    if( c != ' ' )
      str.push_back(c);
  s = str;
}

set<Rule> getProductionRules( string s ) {
  escape(s);
  set<Rule> v;
  string l;
  
  // S->Sa
  for( int i = 3; i <= (int)s.size(); i++ ) {
    if( i == (int)s.size() || s[i] == '|' ) {
      v.insert( Rule( s[0], l, rules ) );
      RuleList[rules] = Rule( s[0], l, rules );
      rules++;
      l.clear();
    }
    else {
      if( isupper(s[i]) )
        nonTerm.insert(s[i]);
      else 
        Term.insert(s[i]);
      literals.insert(s[i]);
      l.push_back(s[i]);
    }
  }
  return v;
}

void FIRST( Rule r ) {
  if( nonTerm.count(r.rhs[0]) )
    _FIRST[r.lhs].insert(r.rhs[0]);
  else {
    for( Rule p : LHS[r.rhs[0]] )
      if( p.lhs != r.lhs ) {
        FIRST(p);
        for( char c : _FIRST[p.lhs] )
          _FIRST[r.lhs].insert(c);
      }
  }
}

void FOLLOW( Rule r ) {
  for( int i = 0; i < (int)r.rhs.size(); i++ )
    if( nonTerm.count(r.rhs[i]) ) {
      if( i+1 >= (int)r.rhs.size() && r.lhs != r.rhs[i+1] ) {
        for( char c : _FOLLOW[r.lhs] )
          _FOLLOW[r.rhs[i]].insert(c);
      }
      else if( i+1 < (int)r.rhs.size() && Term.count(r.rhs[i+1]) )
        _FOLLOW[r.rhs[i]].insert(r.rhs[i+1]);
      else if( i+1 < (int)r.rhs.size() && nonTerm.count(r.rhs[i+1]) ) {
        for( char c : _FOLLOW[r.rhs[i+1]] )
          _FOLLOW[r.rhs[i]].insert(c);
      }
    }
}

struct thread_data {
  set<Rule> sR;
  char ch;
  
  thread_data() {
  }
  thread_data( set<Rule> _sR, char _ch ) {
    sR = _sR;
    ch = _ch;
  }
};

void *rec(void *param) {
  thread_data *data = (thread_data*)param;
  
  set<Rule> v = data->sR;
  char c = data->ch;
  
  set<Rule> idx;
    
  for( Rule r : v ) {
    if( r.rhs[r.ptr] == c ) {
      Rule copy = r;
      copy.ptr++;
      idx.insert(copy);
    }
  }
  
  int initsize = 0;
  while( initsize != (int)idx.size() ) {
    initsize = idx.size();
    set<Rule> toadd;
    for( Rule r : idx )
      if( r.ptr < (int)r.rhs.size() && nonTerm.count(r.rhs[r.ptr]) )
        for( Rule p : LHS[r.rhs[r.ptr]] )
          toadd.insert(p);
    for( Rule r : toadd )
        idx.insert(r);
  }
    
  if( idx.empty() ) 
    return 0;
  
  mtx.lock();
  
  for( Rule r : idx )
    if( r.ptr >= (int)r.rhs.size() ) {
      for( char c : _FOLLOW[r.lhs] )
        ACTION[{getHash(idx),c}] = {'R', r.id};
    }
    
  if( Term.count(c) )
    ACTION[{getHash(v),c}] = {'S',getHash(idx)};
  if( nonTerm.count(c) )
    ACTION[{getHash(v),c}] = {'G',getHash(idx)};
  
  if( explored[getHash(idx)] ) {
    mtx.unlock();
    return 0;
  }
  
  explored[getHash(idx)] = 1;
  
  mtx.unlock();
  
  pthread_attr_t attr;
  void *status;

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  
  pthread_t threads[literals.size()];
  thread_data td[literals.size()];
  int cur = 0;
  for( char t : literals ) {
    td[cur] = thread_data(idx,t);
    pthread_create(&threads[cur], &attr, rec, (void *)&td[cur]);
    cur++;
  }
  
  pthread_attr_destroy(&attr);
  
  for( int i = 0; i < cur; i++ ) {
    int rc = pthread_join(threads[i], &status);
    if (rc) {
       cerr << "Error: unable to join, " << rc << endl;
       exit(-1);
    }
  }

  return 0;
}

void validate( string s ) {
  string backup = s;
  if( s.back() != '$' )
    s.push_back('$');
  escape(s);
  
  cout << "\n[STATE BUFFER ACTION]\n";
  stack< pair<string,int> > st;
  st.push( { "STATE", 0 } );
  
  while(1) {
    int state = st.top().S;
    char c = *s.begin();
    
    cout << setw(backup.size()+2) << s << ":  ";
    
    if( ACTION.find({state,(char)c}) == ACTION.end() ) {
      cout << backup << " - not accepted.\n";
      return;
    }
    
    auto act = ACTION[{state,c}];
    
    if( act.F == 'S' ) {
      st.push( { "TOKEN", c } );
      st.push( { "STATE", act.S } );
      s.erase(s.begin());
    }
    else if( act.F == 'R' ) {
      int id = act.S;
      Rule r = RuleList[id];
      for( int i = 0; i < 2*(int)r.rhs.size(); i++ )
        st.pop();
      int top = st.top().S;
      st.push( { "NT", r.lhs } );
      
      if( ACTION.find({top,r.lhs}) == ACTION.end() ) {
        cout << backup << " ~ not accepted.\n";
        return;
      }
    
      auto now = ACTION[{top,r.lhs}];
      st.push( {"STATE",now.S} );
    }
    
    auto prev = st;
    while(!prev.empty()) {
      if( prev.top().F != "STATE" )
        cout << (char)prev.top().S;
      else
        cout << prev.top().S;
      prev.pop();
    }
    cout << '\n';
    if( s == "$" && st.top().S == ACTION[{0,start}].S ) {
      cout << backup << " ACCEPTED\n";
      return;
    }
  }
  
}

int main() {
  int n;
  cout << "Enter number of Production Rules: ";
  cin >> n;
  cin.ignore();
  
  cout << "Enter start symbol: ";
  cin >> start;
  cin.ignore();
  
  cout << "Enter Production Rules:\n";
  for( int i = 0; i < n; i++ ) {
    string s; getline(cin,s);
    set<Rule> a = getProductionRules(s);
    for( Rule r : a ) {
      LHS[r.lhs].insert(r);
      RuleSet.insert(r);
      FIRST(r);
    }
  }
  
  
  _FOLLOW[start].insert('$');
  for( Rule r : RuleSet ) FOLLOW(r);
  for( Rule r : RuleSet ) FOLLOW(r);
  
  for( char c : nonTerm ) {
    cout << "FOLLOW: " << c << " -> ";
    for( char x : _FOLLOW[c] )
      cout << x << ", ";
    cout << "\n";
  }
  cout << "\n\n";
  
  char _S;
  for( char i = 'A'; i <= 'Z'; i++ )
    if( nonTerm.count(i) == 0 ) {
      _S = i;
      break;
    }
    
  set<Rule> G = RuleSet;
  G.insert( Rule( _S, string(1,start)+"$", -1 ) );
  
  explored[getHash(G)] = 1;
  
  pthread_attr_t attr;
  void *status;

  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  pthread_t threads[literals.size()];
  thread_data td[literals.size()];
  int cur = 0;
  
  for( char c : literals ) {
    td[cur] = thread_data(G,c);
    pthread_create(&threads[cur], &attr, rec, (void *)&td[cur]);
    cur++;
  }
  
  pthread_attr_destroy(&attr);
  
  for( int i = 0; i < cur; i++ ) {
    int rc = pthread_join(threads[i], &status);	
    if (rc) {
       cerr << "Error: unable to join, " << rc << endl;
       exit(-1);
    }
  }
  
  cout << "\n\n[ACTION TABLE]\n";
  for( auto p : ACTION ) {
    cout << "GOTO(I" << p.F.F << "," << p.F.S << ") = " << p.S.F << p.S.S << '\n';
  }
  cout << "Total Table Entries: " << ACTION.size() << "\n\n"; 
  
  while(1) {
    cout << "\nEnter string to validate: ";
    string s; cin >> s;
    validate(s);   
  }
  
}
