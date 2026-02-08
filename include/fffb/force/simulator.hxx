//
//
//      fffb
//      force/simulator.hxx
//

#pragma once

#include <fffb/util/types.hxx>
#include <fffb/joy/wheel.hxx>


namespace fffb
{


////////////////////////////////////////////////////////////////////////////////

struct telemetry_state
{
        timestamp_t                       timestamp { static_cast< timestamp_t >( -1 ) } ;
        timestamp_t         raw_rendering_timestamp { static_cast< timestamp_t >( -1 ) } ;
        timestamp_t        raw_simulation_timestamp { static_cast< timestamp_t >( -1 ) } ;
        timestamp_t raw_paused_simulation_timestamp { static_cast< timestamp_t >( -1 ) } ;

        bool orientation_available { false } ;

        float heading { -1.0 } ;
        float   pitch { -1.0 } ;
        float    roll { -1.0 } ;

        float steering { -1.0 } ;
        float throttle { -1.0 } ;
        float    brake { -1.0 } ;
        float   clutch { -1.0 } ;

        float    speed { -1.0 } ;
        float      rpm { -1.0 } ;
        int       gear { -1   } ;

        int substance_l { -1 } ;
        int substance_r { -1 } ;

        float lateral_accel { 0.0f } ;

        float suspension_deflection_l { 0.0f } ;
        float suspension_deflection_r { 0.0f } ;
} ;

////////////////////////////////////////////////////////////////////////////////

class simulator
{
public:
        constexpr  simulator () noexcept ;
        constexpr ~simulator () noexcept = default ;

        constexpr bool initialize_wheel () noexcept ;

        constexpr void update_forces ( telemetry_state const & _new_state_ ) noexcept ;

        constexpr wheel       & wheel_ref ()       noexcept { return wheel_ ; }
        constexpr wheel const & wheel_ref () const noexcept { return wheel_ ; }
private:
        wheel wheel_ ;

        constexpr void _update_autocenter ( telemetry_state const & _new_state_ ) noexcept ;
        constexpr void _update_constant   ( telemetry_state const & _new_state_ ) noexcept ;
        constexpr void _update_spring     ( telemetry_state const & _new_state_ ) noexcept ;
        constexpr void _update_damper     ( telemetry_state const & _new_state_ ) noexcept ;
        constexpr void _update_trapezoid  ( telemetry_state const & _new_state_ ) noexcept ;

        float prev_deflection_l_ { 0.0f } ;
        float prev_deflection_r_ { 0.0f } ;

        constexpr uti::u8_t _map_rmp_to_freq ( float _rpm_ ) const noexcept
        { return ( 255 - ( _rpm_ / 3000.0f * 255.0f ) ) / 4 ; }

        constexpr uti::u8_t _map_speed_to_freq ( float _speed_ ) const noexcept
        { return ( 255 - ( _speed_ / 160.0f * 255.0f ) ) / 4 ; }
} ;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

constexpr simulator::simulator () noexcept
{
        if( !initialize_wheel() ) FFFB_F_ERR_S( "simulator", "failed initializing wheel!" ) ;
}

////////////////////////////////////////////////////////////////////////////////

constexpr bool simulator::initialize_wheel () noexcept
{
        if( !wheel_ ) return false ;

        wheel_.disable_autocenter() ;
        wheel_.stop_forces() ;

        return true ;
}

////////////////////////////////////////////////////////////////////////////////

constexpr void simulator::update_forces ( telemetry_state const & _new_state_ ) noexcept
{
        _update_autocenter( _new_state_ ) ;
        _update_constant  ( _new_state_ ) ;
        _update_spring    ( _new_state_ ) ;
        _update_damper    ( _new_state_ ) ;
        _update_trapezoid ( _new_state_ ) ;

        wheel_.refresh_forces() ;
}

////////////////////////////////////////////////////////////////////////////////

constexpr void simulator::_update_autocenter ( [[ maybe_unused ]] telemetry_state const & _new_state_ ) noexcept
{}

////////////////////////////////////////////////////////////////////////////////

constexpr void simulator::_update_constant ( telemetry_state const & _new_state_ ) noexcept
{
        double speed = _new_state_.speed < 0.0 ? -_new_state_.speed : _new_state_.speed ;
        double lateral = static_cast< double >( _new_state_.lateral_accel ) ;
        double brake   = static_cast< double >( _new_state_.brake ) ;

        // ramp up over 0-5 m/s (~18 km/h), no force when stopped
        double speed_factor = speed / 5.0 ;
        if( speed_factor > 1.0 ) speed_factor = 1.0 ;
        if( speed_factor < 0.0 ) speed_factor = 0.0 ;

        // high gain: truck lateral accel is typically ±0.5 to ±3 m/s²
        // gain of 32 maps 3 m/s² -> 96 units offset from center = strong pull
        double raw_force = lateral * 32.0 * speed_factor ;

        // reduce force on heavy braking (grip loss simulation)
        double brake_factor = brake > 0.7 ? 0.6 : 1.0 ;

        double amplitude = 128.0 + raw_force * brake_factor ;

        // clamp — allow nearly full range for strong forces
        if( amplitude <  8.0 ) amplitude =  8.0 ;
        if( amplitude > 248.0 ) amplitude = 248.0 ;

        wheel_.constant_force() = wheel::default_const_f ;

        if( speed < 0.1 )
        {
                wheel_.constant_force().enabled = false ;
        }
        else
        {
                wheel_.constant_force().enabled   = true ;
                wheel_.constant_force().amplitude = static_cast< uti::u8_t >( amplitude ) ;
        }
}

////////////////////////////////////////////////////////////////////////////////

constexpr void simulator::_update_spring ( telemetry_state const & _new_state_ ) noexcept
{
        double speed = _new_state_.speed < 0.0 ? -_new_state_.speed : _new_state_.speed ;

        wheel_.spring_force() = wheel::default_spring_f ;

        if( speed < 0.10 )
        {
                wheel_.spring_force().enabled = false ;
        }
        else
        {
                wheel_.spring_force().enabled = true ;

                // small dead zone at center
                wheel_.spring_force().dead_start = 126 ;
                wheel_.spring_force().dead_end   = 130 ;

                // slope: max is 7 (3-bit). aggressive ramp with speed
                uti::u8_t slope ;
                if( speed <= 3.0 )
                {
                        slope = 2 ;
                }
                else if( speed <= 15.0 )
                {
                        // ramp from 2 to 5 over 3-15 m/s
                        slope = static_cast< uti::u8_t >( 2.0 + ( speed - 3.0 ) / 12.0 * 3.0 ) ;
                }
                else
                {
                        // ramp from 5 to 7 over 15-30 m/s, capped
                        double s = 5.0 + ( speed - 15.0 ) / 15.0 * 2.0 ;
                        if( s > 7.0 ) s = 7.0 ;
                        slope = static_cast< uti::u8_t >( s ) ;
                }
                wheel_.spring_force().slope_left  = slope ;
                wheel_.spring_force().slope_right = slope ;

                // amplitude: strong centering force that ramps up with speed
                // parking (< 5 m/s): light but present
                // city (5-20 m/s): solid centering
                // highway (20+ m/s): very strong centering
                double amp ;
                if( speed <= 5.0 )
                {
                        amp = 64.0 + speed * 12.0 ;   // 64 -> 124
                }
                else if( speed <= 20.0 )
                {
                        amp = 124.0 + ( speed - 5.0 ) * 5.0 ;  // 124 -> 199
                }
                else
                {
                        amp = 199.0 + ( speed - 20.0 ) * 2.0 ;  // 199 -> ...
                }
                if( amp > 240.0 ) amp = 240.0 ;
                wheel_.spring_force().amplitude = static_cast< uti::u8_t >( amp ) ;
        }
}

////////////////////////////////////////////////////////////////////////////////

constexpr void simulator::_update_damper ( telemetry_state const & _new_state_ ) noexcept
{
        double speed = _new_state_.speed < 0.0 ? -_new_state_.speed : _new_state_.speed ;
        double brake = static_cast< double >( _new_state_.brake ) ;

        wheel_.damper_force() = wheel::default_damper_f ;
        wheel_.damper_force().enabled = true ;

        // base slope: ramps faster to give noticeable resistance
        double base_slope = speed / 10.0 * 5.0 ;
        if( base_slope < 2.0 ) base_slope = 2.0 ;
        if( base_slope > 6.0 ) base_slope = 6.0 ;

        // braking increases damping (weight transfer to front = heavier steering)
        int brake_bonus = brake > 0.3 ? 1 : 0 ;

        int slope = static_cast< int >( base_slope ) + brake_bonus ;
        if( slope > 7 ) slope = 7 ;

        wheel_.damper_force().slope_left  = static_cast< uti::u8_t >( slope ) ;
        wheel_.damper_force().slope_right = static_cast< uti::u8_t >( slope ) ;
}

////////////////////////////////////////////////////////////////////////////////

constexpr void simulator::_update_trapezoid ( telemetry_state const & _new_state_ ) noexcept
{
        double speed = _new_state_.speed < 0.0 ? -_new_state_.speed : _new_state_.speed ;
        int sub_l = _new_state_.substance_l ;
        int sub_r = _new_state_.substance_r ;

        wheel_.trapezoid_force() = wheel::default_trap_f ;

        bool offroad = ( sub_l != 0 ) || ( sub_r != 0 ) ;

        if( !offroad || speed < 0.5 )
        {
                wheel_.trapezoid_force().enabled = false ;
                prev_deflection_l_ = _new_state_.suspension_deflection_l ;
                prev_deflection_r_ = _new_state_.suspension_deflection_r ;
                return ;
        }

        wheel_.trapezoid_force().enabled = true ;

        // suspension deflection delta for bump detection
        double delta_l = _new_state_.suspension_deflection_l - prev_deflection_l_ ;
        double delta_r = _new_state_.suspension_deflection_r - prev_deflection_r_ ;
        if( delta_l < 0.0 ) delta_l = -delta_l ;
        if( delta_r < 0.0 ) delta_r = -delta_r ;
        double defl_delta = delta_l > delta_r ? delta_l : delta_r ;

        prev_deflection_l_ = _new_state_.suspension_deflection_l ;
        prev_deflection_r_ = _new_state_.suspension_deflection_r ;

        // amplitude range: mild vibration around center (128)
        // expand range when suspension is bouncing
        double bump_extra = defl_delta > 0.005 ? 4.0 : 0.0 ;

        double amp_max = 116.0 - bump_extra ;
        double amp_min = 140.0 + bump_extra ;
        if( amp_max < 96.0  ) amp_max = 96.0  ;
        if( amp_min > 160.0 ) amp_min = 160.0 ;

        wheel_.trapezoid_force().amplitude_max = static_cast< uti::u8_t >( amp_max ) ;
        wheel_.trapezoid_force().amplitude_min = static_cast< uti::u8_t >( amp_min ) ;

        // slope and timing scale with speed for more pronounced vibration at speed
        double speed_scale = speed / 30.0 ;
        if( speed_scale > 1.0 ) speed_scale = 1.0 ;

        uti::u8_t slope = static_cast< uti::u8_t >( 2.0 + speed_scale * 6.0 ) ;
        if( slope > 0x0F ) slope = 0x0F ;

        wheel_.trapezoid_force().slope_step_x = slope ;
        wheel_.trapezoid_force().slope_step_y = slope ;

        // shorter periods at speed for faster oscillation
        uti::u8_t t_val = static_cast< uti::u8_t >( 48.0 - speed_scale * 32.0 ) ;
        if( t_val < 8 ) t_val = 8 ;

        wheel_.trapezoid_force().t_at_max = t_val ;
        wheel_.trapezoid_force().t_at_min = t_val ;
}

////////////////////////////////////////////////////////////////////////////////


} // namespace fffb
