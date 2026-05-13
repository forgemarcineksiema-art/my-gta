# Manual Playtest Checklist

Status: CURRENT
Last verified against code: 2026-05-10
Owner: gameplay QA
Source of truth for: manual smoke checks after controls, camera, vehicle, and slice changes

Run this after every controls/camera change.

## v0.8.1 Readability Gate

- Start the game through `.\tools\run_dev.ps1` and confirm the window title starts with `Blok 13 |` and includes build stamp, DevTools status, data root, and executable path.
- Press `F11` twice. Expected: fullscreen toggles on and off without losing input.
- Press `Esc` and `F2`. Expected: both lock/unlock mouse capture; neither immediately quits the game.
- Start the intro at Bogus. Expected: objective text is short and includes distance to target.
- Walk to Zenon's shop without prior map knowledge. Expected: green objective arrow/marker makes the route readable.
- Try to enter the gruz before finishing the shop/Bogus loop. Expected: prompt or line explains `Najpierw Zenon i Bogus`.
- Complete the loop: Bogus -> Zenon's shop -> Bogus -> gruz -> shop -> chase -> parking.
- After the intro completion line, confirm a system hint points to `Paragon Grozy` at Bogus and the Bogus prompt can start it.
- Confirm subtitles stay in the lower subtitle box, wrap if needed, and do not float around the world.
- Confirm Przypal and reaction toasts add flavor without covering mission/chase/fail UI.
- Confirm the first complaint is not "nie wiem gdzie jest sklep" or "nie moge wsiasc do auta".

## Feel Gate Thresholds

- Debug target lag should usually stay below 0.75m while sprinting straight.
- Camera boom gap may spike near walls, but the player must remain visible and the boom should recover within about 1.5s after the wall clears.
- Sprint release should stop within roughly one body length and feel near-idle in under 0.3s.
- Quick reverse input should cancel the old direction in one short beat, without a long forward slide.
- Wall/corner bumps may wobble, but should not lock control longer than about 0.25s.
- Handbrake slip should be visible during the turn, then mostly recover within about 1s after release.
- Damaged gruz may be weaker and noisier, but must still accelerate, steer, brake, and boost predictably.

## Free Roam

- Play 10 minutes without starting the mission.
- Walk around the block using mouse camera and WASD.
- Hold Ctrl while moving and confirm the character slows into a controlled walk.
- Sprint with Shift in straight lines and around corners.
- Move the mouse 10 short swipes left/right and confirm the view turns clearly without twitching past the intended direction.
- After moving the mouse during sprint, keep sprinting for about one second and confirm the camera does not immediately fight back.
- Keep sprinting straight for 10 seconds and confirm the camera follows tightly instead of feeling like it is tied to the player by elastic.
- While jogging forward, tap the opposite direction and confirm the character turns without a long forward slide.
- Use F1 and confirm debug shows camera mode, yaw/pitch, boom/full boom, target lag, and player speed state.
- Jump with Space while standing, walking, and sprinting.
- Press Space just before landing and confirm the jump buffer makes the next jump feel forgiving.
- Confirm movement follows camera direction, not world axes.
- With debug overlay on, compare the blue camera-forward line, red camera-right line, and white movement velocity line while pressing W+A, W+D, S+A, and S+D.
- Sprint during the chase and confirm it feels more panicked without losing control.
- Step repeatedly onto the low curb/test platform near the parking area.
- Sprint across the test ramp and confirm the character rises smoothly.
- Run diagonally into building corners and confirm the character slides instead of sticking.
- Hit a corner at speed and confirm any wobble/stagger is short, readable, and not a long input lock.

## Vehicle

- Enter and exit the car 5 times.
- Drive the road loop for 10 minutes.
- Use Space as handbrake only while inside the car.
- Hold Shift while accelerating and confirm the car gets louder/more nervous without becoming unreadable.
- Tap H repeatedly and confirm horn pulse/audio has a cooldown and does not move the car.
- Tap LMB while driving and confirm it uses the same horn/action family as H.
- Use E and F near the car and confirm both can enter/exit without confusing other prompts.
- Stand where the NPC prompt and car prompt are both visible; confirm E chooses the NPC/mission prompt while F chooses the car.
- Do 20 tight parking-lot turns without handbrake and confirm low-speed steering is readable.
- Do 20 handbrake turns and confirm slip is funny but still recoverable.
- Brake from forward speed to zero 10 times, then hold S and confirm reverse starts only after stopping.
- While reversing, press A and D separately and confirm steering feels screen/arcade-intuitive rather than physically inverted.
- Hit walls/props 10 times and confirm Gruz condition drops through readable bands without randomly disabling the car.
- Keep driving after reaching a bad condition band and confirm the car is weaker/smokier, not unusable.
- After handbrake turns, release Space and confirm slip/instability cools down instead of fighting the next turn.
- Damage the car heavily, then test W, S, A/D, Space, and Shift; confirm bad condition changes flavor, not basic readability.
- Confirm the camera recenters behind the car without snapping.
- Drive near walls and confirm vehicle camera collision keeps the car visible.

## Chase

- Repeat the chase 5 times.
- Confirm the pursuer creates pressure without instant failure.
- Confirm escape progress is readable.
- Confirm high chase pressure subtly changes camera/FOV/shake without making steering confusing.
- Confirm fail/retry UI remains readable.

## Przypal / World Reactions

Use this section after v0.7 is implemented.

- Honk under the block 5 times. Expected: one small Przypal bump, a cooldown between pulses, no instant jump to high heat.
- Hold Shift boost in the car for 5 seconds. Expected: occasional noise heat, not per-frame heat pumping.
- Hit a wall/prop hard once. Expected: `PropertyDamage` heat only when the impact is clearly strong; light scraping should not count.
- Keep rubbing the car along a wall for 5 seconds. Expected: no runaway Przypal; stack/emit cooldown should dampen repeat events.
- Reach the shop objective. Expected: one shop-flavored reaction or HUD-only pulse, not repeated every frame.
- Trigger chase pressure near public space. Expected: `ChaseSeen` appears periodically at most, not every frame.
- Wait 2 seconds after the last chaos event. Expected: Przypal waits briefly, then cools down at a visible but not nervous pace.
- Watch reaction lines from the same source. Expected: the same line should not repeat twice in a row.
- Trigger chaos while mission/chase/fail text is critical. Expected: low-priority gossip is blocked; high-priority alarm can become HUD-only.
- Confirm HUD in 720p shows Przypal band, value, and last reason without covering the objective panel.
- Confirm Przypal feedback does not change `WASD`, camera yaw, player motor behavior, vehicle steering, or chase readability.

## Pass Criteria

- The player can describe controls without asking for help.
- Camera-relative movement feels natural after 30 seconds.
- Walk, jog, and sprint feel like distinct on-foot states.
- Fast direction changes do not feel like the character is skating away from input.
- Low curbs and ramps do not require jumping.
- Walls stop the player without snapping or glueing them in place.
- The camera does not sit behind solid geometry when close to a wall.
- Comedy feedback is visible but does not steal control for more than a brief moment.
- Entering/exiting car does not disorient the player.
- World reactions add pressure without stealing control or spamming subtitles.
- The first complaint is no longer "sterowanie jest popierdolone".
