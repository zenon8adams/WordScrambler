#include <vector>
#include <unordered_map>
#include <random>
#include <iostream>
#include <iterator>
#include <regex>
#include <fstream>
#include <sstream>
#include "utility.hpp"

struct Coord
{
	int x{ NEG_INF },
	    y{ NEG_INF };
};

class WordScrambler
{
public:
	WordScrambler( const std::vector<std::string>& words )
	: m_words( words )
	{
		setup();
	}
	auto rearrange()
	{
		// TODO: Add smart word merger.
		computeFreeSpace();
		capitalizeWords();
		scramble();
		auto n_inserted = positionWords();
		addNoise();
		return n_inserted;
	}
	auto puzzle() const
	{
		return m_puzzle;
	}
	auto words() const
	{
		return m_cap_words;
	}
private:
	
	struct PosMetaData
	{
		Coord pos;
		int freespace[ 8 ] = {};
		bool full = false;
	};
	
	void setup()
	{
		srand( time( 0));
		createPuzzleSkeleton();
	}
	
	void createPuzzleSkeleton()
	{
		std::size_t dim = 0, avg_len = 0, max_len = 0;
		for( const auto& w : m_words )
		{
			max_len = std::max( max_len, w.size() );
			avg_len += w.size();
		}
        max_len += 2;
		dim = std::max( max_len, (std::size_t)std::sqrt( 2.5*avg_len ) );
		m_puzzle.resize( dim );
		m_pos_col.resize( dim );
		for( std::size_t i = 0; i < m_puzzle.size(); ++i )
		{
			m_puzzle[ i].resize( dim );
			m_pos_col[ i].resize( dim );
			for( std::size_t j = 0; j < m_puzzle.size(); ++j )
				m_pos_col[ i][ j] = { int(i), int(j) };
		}
		m_nrow = m_ncol = m_puzzle.size();
	}
	
	void addNoise()
	{
		for( std::size_t i = 0; i < m_ncol; ++i )
			for( std::size_t j = 0; j < m_nrow; ++j )
			{
				if( !m_puzzle[ i][ j] )
					m_puzzle[ i][ j] = 'A' + rand() % 26;
			}
	}
	
	void capitalizeWords()
	{
		for( auto& w : m_words )
			std::transform( w.begin(), w.end(), w.begin(), toupper );
		m_cap_words = m_words;
	}
	
	std::size_t positionWords()
	{
		std::size_t n_inserted = 0;
		for( const auto& w : m_words )
		{
			if( try_position( w ) )
			{
				++n_inserted;
				computeFreeSpace();
			}
		}
		return n_inserted;
	}

	auto vacant( std::size_t size )
	{
		std::pair<int, PosMetaData> pos_m{ -1, {} };
		std::size_t n_trials = 100, i, j;
		while( n_trials-- )
		{
			i = rand() % m_ncol;
			j = rand() % m_nrow;
			if( m_pos_col[ i][ j].full )
				continue ;
			unsigned char is_full = 0;
	    	for( int k = 0; k < 8; ++k )
	    	{
	    		int idx = rand() % 8;
	    		auto freespace = m_pos_col[ i][ j].freespace;
	    		is_full |= ( freespace[ idx ] ? 0 : 1 ) << k;
	    		if( is_full == 0xFF )
	    		{
	    			m_pos_col[ i][ j].full = true;
	    			break;
	    		}
	    		if( freespace[ idx ] >= size )
	    			return std::pair{ idx, m_pos_col[ i][ j] };
	    	}
	    }
	    return pos_m;
	}
	
	bool try_position( const std::string& word )
	{
		auto viable = vacant( word.size() );
		if( viable.first != -1 )
		{
			insert( viable, word );
			return true;
		}
		return false;
	}
	
	void insert( std::pair<int, PosMetaData> pos_m, const std::string& word )
	{
		std::size_t wsize = word.size();
		auto& ipos = pos_m.second.pos;
		auto& freespace = pos_m.second.freespace;
		for( std::size_t idx = 0; idx < wsize; ++idx )
		{
			m_puzzle[ ipos.x ][ ipos.y ] = word[ idx ];
			ipos = m_dirlookup[ Dir( pos_m.first+1 ) ]( ipos );
		}
	}
	
	void scramble()
	{
		std::random_device rd;
		std::mt19937_64 m_gen( rd() );
		std::uniform_int_distribution<int> dist( 1, m_words.size() );
		std::size_t nrev = dist( m_gen );
		while( nrev-- )
		{
			auto idx = dist( m_gen ) - 1;
            if( idx < m_words.size())
			    m_words[ idx ] = reversed( m_words[ idx ] );
		}
		std::sort( m_words.begin(), m_words.end(), std::greater<std::string>() );
	}
	
	void computeFreeSpace()
	{
		for( std::size_t i = 0; i < m_ncol; ++i )
			for( std::size_t j = 0; j < m_nrow; ++j )
				setFreeSpace( m_pos_col[ i][ j] );
	}
	
	void setFreeSpace( PosMetaData& pos_m )
	{
		for( int i = 0; i < 8; ++i )
		{
			int nfree = 0;
			Coord pos = pos_m.pos, next;
			while( isValidDir( next = m_dirlookup[ Dir(i+1) ]( pos ) ) 
			       && !m_puzzle[ pos.x ][ pos.y] )
			{
				++nfree;
				pos = next;
			}
			pos_m.freespace[ i] = nfree;
		}
	}
	
	bool isValidDir( Coord pos )
	{
		return pos.x >= 0 && pos.x < m_ncol &&
	    	   pos.y >= 0 && pos.y < m_nrow;
	}
	std::vector<std::string> m_puzzle, m_words, m_cap_words;
	std::size_t m_nrow{ 0}, m_ncol{ 0};
	std::vector<std::vector<PosMetaData>> m_pos_col;
	static std::unordered_map<Dir, std::function<Coord(Coord)>> m_dirlookup;
};

std::unordered_map<Dir, 
std::function<Coord(Coord)>> WordScrambler::m_dirlookup
{
	{ Dir::NL, []( auto pos ){ return Coord{                  }; } },
	{ Dir::NT, []( auto pos ){ return Coord{ pos.x-1, pos.y   }; } },
	{ Dir::ST, []( auto pos ){ return Coord{ pos.x+1, pos.y   }; } },
	{ Dir::ET, []( auto pos ){ return Coord{ pos.x,   pos.y+1 }; } },
	{ Dir::WT, []( auto pos ){ return Coord{ pos.x,   pos.y-1 }; } },
	{ Dir::NE, []( auto pos ){ return Coord{ pos.x-1, pos.y+1 }; } },
	{ Dir::SE, []( auto pos ){ return Coord{ pos.x+1, pos.y+1 }; } },
	{ Dir::NW, []( auto pos ){ return Coord{ pos.x-1, pos.y-1 }; } },
	{ Dir::SW, []( auto pos ){ return Coord{ pos.x+1, pos.y-1 }; } }
};

auto gemExtractor( std::ifstream& gstrm, const std::string& limit)
{
    std::regex g_regex("[a-zA-Z]{" + limit + ",}");
    std::stringstream sstrm; sstrm << gstrm.rdbuf();
    std::string strm( sstrm.str());
    auto g_begin  = std::sregex_iterator( strm.cbegin(),
                                          strm.cend(), g_regex);
    auto g_end = std::sregex_iterator();
    std::vector<std::string> g_collector( std::distance( g_begin, g_end));
    size_t n = 0;
    for ( auto i = g_begin; i != g_end; ++i)
        g_collector[ n++] = i->str();

    return g_collector; 
}

void confirmInteger( char *text)
{
    while( *text)
    {
        if( !isdigit( *text++))
            throw std::logic_error( "Expected an integer");
    }
}

auto gemCollector( char **pf)
{
    std::vector<std::string> gems;

    std::string limit( (*pf) + 1);
    try
    {
        confirmInteger( (*pf) + 1);
    }
    catch( const std::exception& e)
    {
        std::cerr<< "ERROR! " << e.what() <<'\n';
        exit( 1);
    }

    while( *++pf)
    {
        std::ifstream strm( *pf, std::ios::in);
        if( strm.good())
        {
            auto cgems = gemExtractor( strm, limit);
            std::copy( cgems.begin(), cgems.end(), std::back_inserter( gems));
            strm.close();
        }
    }

    return gems;
}



int main( int ac, char *av[])
{
    if( ac == 1 || ac == 2)
    {
        std::cerr << "Usage: " << av[ 0]
                  << " -n [FILE]...\n"
                  << "Where: n - Shortest word length\n";
        exit( 1);
    }

    auto words{ gemCollector( ++av)};
	WordScrambler ws( words );
	auto n_inserted = ws.rearrange();
	auto puzzle = ws.puzzle();
	std::cout << "Inserted: " << n_inserted <<'\n'
	          << "Remaining: " << ( words.size() - n_inserted ) <<"\n\n";
	for( const auto& row : puzzle )
	{
		for( const auto& c : row )
			std::cout << c <<' ';
		putchar('\n');
	}
}
