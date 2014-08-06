#include <SFGUI/Renderers/VertexArrayRenderer.hpp>
#include <SFGUI/RendererBatch.hpp>
#include <SFGUI/RendererViewport.hpp>
#include <SFGUI/Signal.hpp>
#include <SFGUI/Primitive.hpp>
#include <SFGUI/PrimitiveVertex.hpp>

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/OpenGL.hpp>

namespace sfg {

VertexArrayRenderer::VertexArrayRenderer() :
	m_last_vertex_count( 0 ),
	m_last_index_count( 0 ),
	m_alpha_threshold( 0.f ),
	m_dirty( true ),
	m_cull( false ) {
}

VertexArrayRenderer::Ptr VertexArrayRenderer::Create() {
	return Ptr( new VertexArrayRenderer );
}

const std::string& VertexArrayRenderer::GetName() const {
	static const std::string name( "Vertex Array Renderer" );
	return name;
}

void VertexArrayRenderer::DisplayImpl() const {
	if( m_alpha_threshold > 0.f ) {
		glAlphaFunc( GL_GREATER, m_alpha_threshold );
		glEnable( GL_ALPHA_TEST );
	}

	if( m_dirty ) {
		// Disclaimer:
		// const_cast IS safe to use in ANY non-static method of
		// Renderer. Because we make sure that the only instance
		// that can be constructed is the singleton instance and
		// that the singleton instance is non-const, we don't have
		// to fear that some member variable is non-mutable. This
		// is a nice hack to please certain people who require const
		// methods in the most exotic places without having to declare
		// half the member variables and methods as mutable. Then
		// again this might be all wrong...

		// Refresh array data if out of sync
		const_cast<VertexArrayRenderer*>( this )->RefreshArray();
	}

	glVertexPointer( 2, GL_FLOAT, 0, &m_vertex_data[0] );
	glColorPointer( 4, GL_UNSIGNED_BYTE, 0, &m_color_data[0] );
	glTexCoordPointer( 2, GL_FLOAT, 0, &m_texture_data[0] );

	// Not needed, constantly kept enabled by SFML... -_-
	//glEnableClientState( GL_VERTEX_ARRAY );
	//glEnableClientState( GL_COLOR_ARRAY );
	//glEnableClientState( GL_TEXTURE_COORD_ARRAY );

	glEnable( GL_SCISSOR_TEST );

	auto current_atlas_page = 0;

	sf::Texture::bind( m_texture_atlas[0].get() );

	for( const auto& batch : m_batches ) {
		auto viewport = batch.viewport;

		if( batch.custom_draw ) {
			auto destination = static_cast<sf::Vector2i>( viewport->GetDestinationOrigin() );
			auto size = static_cast<sf::Vector2i>( viewport->GetSize() );

			glViewport( destination.x, m_window_size.y - destination.y - size.y, size.x, size.y );

			// Draw canvas.
			( *batch.custom_draw_callback )();

			glViewport( 0, 0, m_window_size.x, m_window_size.y );

			sf::Texture::bind( m_texture_atlas[static_cast<std::size_t>( current_atlas_page )].get() );
		}
		else {
			if( viewport && ( ( *viewport ) != ( *m_default_viewport ) ) ) {
				auto destination_origin = static_cast<sf::Vector2i>( viewport->GetDestinationOrigin() );
				auto size = static_cast<sf::Vector2i>( viewport->GetSize() );

				glScissor(
					destination_origin.x,
					m_window_size.y - ( destination_origin.y + size.y ),
					size.x,
					size.y
				);
			}
			else {
				glScissor( 0, 0, m_window_size.x, m_window_size.y );
			}

			if( batch.index_count ) {
				if( batch.atlas_page != current_atlas_page ) {
					current_atlas_page = batch.atlas_page;

					sf::Texture::bind( m_texture_atlas[static_cast<std::size_t>( current_atlas_page )].get() );
				}

				glDrawElements(
					GL_TRIANGLES,
					batch.index_count,
					GL_UNSIGNED_INT,
					reinterpret_cast<const char*>( &m_index_data[0] ) + static_cast<unsigned int>( batch.start_index * static_cast<int>( sizeof( GLuint ) ) )
				);
			}
		}
	}

	glDisable( GL_SCISSOR_TEST );

	//glDisableClientState( GL_TEXTURE_COORD_ARRAY );
	//glDisableClientState( GL_COLOR_ARRAY );
	//glDisableClientState( GL_VERTEX_ARRAY );

	m_dirty = false;

	if( m_alpha_threshold > 0.f ) {
		glDisable( GL_ALPHA_TEST );
		glAlphaFunc( GL_GREATER, 0.f );
	}
}

void VertexArrayRenderer::RefreshArray() {
	SortPrimitives();

	m_vertex_data.clear();
	m_color_data.clear();
	m_texture_data.clear();
	m_index_data.clear();

	m_vertex_data.reserve( static_cast<std::size_t>( m_vertex_count ) );
	m_color_data.reserve( static_cast<std::size_t>( m_vertex_count ) );
	m_texture_data.reserve( static_cast<std::size_t>( m_vertex_count ) );
	m_index_data.reserve( static_cast<std::size_t>( m_index_count ) );

	m_batches.clear();

	m_last_vertex_count = 0;
	m_last_index_count = 0;

	auto primitives_size = m_primitives.size();

	// Default viewport
	priv::RendererBatch current_batch;
	current_batch.viewport = m_default_viewport;
	current_batch.atlas_page = 0;
	current_batch.start_index = 0;
	current_batch.index_count = 0;
	current_batch.min_index = 0;
	current_batch.max_index = m_vertex_count - 1;
	current_batch.custom_draw = false;

	sf::FloatRect window_viewport( 0.f, 0.f, static_cast<float>( m_window_size.x ), static_cast<float>( m_window_size.y ) );

	const auto max_texture_size = GetMaxTextureSize();
	const auto default_texture_size = m_texture_atlas[0]->getSize();

	for( std::size_t primitive_index = 1; primitive_index != primitives_size + 1; primitive_index += 1 ) {
		Primitive* primitive = m_primitives[primitive_index - 1].get();

		primitive->SetSynced();

		if( !primitive->IsVisible() ) {
			continue;
		}

		sf::Vector2f position_transform( primitive->GetPosition() );

		auto viewport = primitive->GetViewport();

		auto viewport_rect = window_viewport;

		// Check if primitive needs to be rendered in a custom viewport.
		if( viewport && ( ( *viewport ) != ( *m_default_viewport ) ) ) {
			sf::Vector2f destination_origin( viewport->GetDestinationOrigin() );
			sf::Vector2f size( viewport->GetSize() );

			position_transform += ( destination_origin - viewport->GetSourceOrigin() );

			if( m_cull ) {
				viewport_rect.left = destination_origin.x;
				viewport_rect.top = destination_origin.y;
				viewport_rect.width = size.x;
				viewport_rect.height = size.y;
			}
		}

		const std::shared_ptr<Signal>& custom_draw_callback( primitive->GetCustomDrawCallback() );

		if( custom_draw_callback ) {
			// Start a new batch.
			current_batch.max_index = m_last_vertex_count ? ( m_last_vertex_count - 1 ) : 0;
			m_batches.push_back( current_batch );

			// Mark current_batch custom draw batch.
			current_batch.viewport = viewport;
			current_batch.start_index = 0;
			current_batch.index_count = 0;
			current_batch.min_index = 0;
			current_batch.max_index = 0;
			current_batch.custom_draw = true;
			current_batch.custom_draw_callback = custom_draw_callback;

			// Start a new batch.
			m_batches.push_back( current_batch );

			// Reset current_batch to defaults.
			current_batch.viewport = m_default_viewport;
			current_batch.start_index = m_last_index_count;
			current_batch.index_count = 0;
			current_batch.min_index = m_last_vertex_count ? ( m_last_vertex_count - 1 ) : 0;
			current_batch.custom_draw = false;
		}
		else {
			// Process primitive's vertices and indices
			const std::vector<PrimitiveVertex>& vertices( primitive->GetVertices() );
			const std::vector<GLuint>& indices( primitive->GetIndices() );

			sf::Vector2f position( 0.f, 0.f );

			sf::FloatRect bounding_rect( 0.f, 0.f, 0.f, 0.f );

			auto atlas_page = 0;

			const auto vertices_size = vertices.size();
			sf::Vector2f normalizer;

			for( std::size_t index = 0; index < vertices_size; ++index ) {
				const auto vertex = vertices[index];
				position.x = vertex.position.x + position_transform.x;
				position.y = vertex.position.y + position_transform.y;

				m_vertex_data.push_back( position );
				m_color_data.push_back( vertex.color );

				// The bound texture can only change between triangles.
				if( index % 3 == 0 ) {
					atlas_page = static_cast<int>( vertex.texture_coordinate.y ) / max_texture_size;
					auto texture_size = ( vertex.texture_coordinate.y <= 1.f ) ? default_texture_size : m_texture_atlas[static_cast<std::size_t>( atlas_page )]->getSize();

					// Used to normalize texture coordinates.
					normalizer.x = 1.f / static_cast<float>( texture_size.x );
					normalizer.y = 1.f / static_cast<float>( texture_size.y );
				}

				// Normalize SFML's pixel texture coordinates.
				m_texture_data.emplace_back( vertex.texture_coordinate.x * normalizer.x, static_cast<float>( static_cast<int>( vertex.texture_coordinate.y ) % max_texture_size ) * normalizer.y );

				// Update the bounding rect.
				if( m_cull ) {
					if( position.x < bounding_rect.left ) {
						bounding_rect.width += bounding_rect.left - position.x;
						bounding_rect.left = position.x;
					}
					else if( position.x > bounding_rect.left + bounding_rect.width ) {
						bounding_rect.width = position.x - bounding_rect.left;
					}

					if( position.y < bounding_rect.top ) {
						bounding_rect.height += bounding_rect.top - position.y;
						bounding_rect.top = position.y;
					}
					else if( position.y > bounding_rect.top + bounding_rect.height ) {
						bounding_rect.height = position.y - bounding_rect.top;
					}
				}
			}

			if( m_cull && !viewport_rect.intersects( bounding_rect ) ) {
				m_vertex_data.resize( static_cast<std::size_t>( m_last_vertex_count ) );
				m_color_data.resize( static_cast<std::size_t>( m_last_vertex_count ) );
				m_texture_data.resize( static_cast<std::size_t>( m_last_vertex_count ) );
			}
			else {
				for( const auto& index : indices ) {
					m_index_data.push_back( m_last_vertex_count + static_cast<int>( index ) );
				}

				// Check if we need to start a new batch.
				if( ( ( *viewport ) != ( *current_batch.viewport ) ) || ( atlas_page != current_batch.atlas_page ) ) {
					current_batch.max_index = m_last_vertex_count ? ( m_last_vertex_count - 1 ) : 0;
					m_batches.push_back( current_batch );

					// Reset current_batch to defaults.
					current_batch.viewport = viewport;
					current_batch.atlas_page = atlas_page;
					current_batch.start_index = m_last_index_count;
					current_batch.index_count = 0;
					current_batch.min_index = m_last_vertex_count ? ( m_last_vertex_count - 1 ) : 0;
					current_batch.custom_draw = false;
				}

				current_batch.index_count += static_cast<int>( indices.size() );

				m_last_vertex_count += static_cast<GLsizei>( vertices.size() );
				m_last_index_count += static_cast<GLsizei>( indices.size() );
			}
		}
	}

	current_batch.max_index = m_last_vertex_count ? ( m_last_vertex_count - 1 ) : 0;
	m_batches.push_back( current_batch );
}

void VertexArrayRenderer::TuneAlphaThreshold( float alpha_threshold ) {
	m_alpha_threshold = alpha_threshold;
}

void VertexArrayRenderer::TuneCull( bool enable ) {
	m_cull = enable;
}

void VertexArrayRenderer::InvalidateImpl( unsigned char /*datasets*/ ) {
	m_dirty = true;
}

}
